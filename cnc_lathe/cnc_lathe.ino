volatile uint8_t q = 1, w = 0, c = 0; //  q, w - для обработки прерывания с клавиатуры, c - передает в loop команду от клавиатуры
volatile bool esc = false;    // флаг выброса из штатных проходов резца. Выбрасывает по true. Сменяется на true при нажатии любой клавиши кроме Пробел и Enter
volatile bool pause = false;  // флаг заморозки штатных проходов резца. Замораживается по true. Сменяется на true только при нажатии пробела, обратно - по Enter

// назначение пинов концевикам
byte endPinLeft = 15;
byte endPinBack = 16;
byte endPinForward = 17;
byte endPinRight = 18;
// назначение пинов моторам
byte pinPulseX = 9;
byte pinPulseY = 10;
byte pinDirX = 11;
byte pinDirY = 12;
// назначение констант
int delayCutX = 8;
int delayCutY = 10;
int delayRunX = 1;
int delayRunY = 1;
int edgeStep = 44 *4;        // шаг стандартного ребра в импульсах (к-во десяток * 4)
int depth = 5555;           // глубина погружения резца в импульсах - от заднего бордюра
int miniDepth = 180;        // глубина погружения резца в импульсах - для снятия продольного слоя
int endX = 5030 *4;
int homeX = 320 *4 ; // дописать переопределение для заготовок: перевернутой8, укороченой9, перевернутойУкороченой0
int homeY = 720;     // (замеряем по факту)
//int borderLeft = 300 *4;   // бордюр слева
//int borderBack = 0;       // бордюр задний - в нулевой точке
//int finishLineX = 1270;   // зона срабатывания левого концевика (на 1,25 мм [50импульсов] правее бордюра)
//int finishLineY = 500;    // зона срабатывания тыльного концевика
// назначение стартовых значений координатам правого угла правого резца
volatile int currentX = 6000 *4;
volatile int currentY = 6000;
// назначение массива координат ребер изделия по X (различаются для разных резцов)
volatile int edges[100];

void setup() {
  Serial.begin(57600);
  Serial.println(11);
  // инициация встроенного диода для теста клавиатуры
  pinMode(LED_BUILTIN, OUTPUT);
  // инициация пинов на прием сигнала от концевиков (4 штуки)
  pinMode(15, INPUT);   // A1 - leftEnd
  pinMode(16, INPUT);   // A2 - backEnd
  pinMode(17, INPUT);   // A3 - forwardEnd
  pinMode(18, INPUT);   // A4 - rightEnd
  // инициация пинов на передачу управляющих команд моторам (4 штуки)
  pinMode(9, OUTPUT);           
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  // инициация прерывания 1 - всегда ожидается на D3 (прерывания 0 - на D2)),
  // функция keyPress вызывается при нисходящем срабатывании сигнала на D3
  attachInterrupt(1, keyPress, FALLING);
  // инициализация координат ребер домашней позицией по X
//  resetCutter();
//  goHomeX();
//  test();
}

void loop() {                 // вход в основной рабочий цикл
  c = 0;  q = 1; w = 0;       // обнуление буферов для ввода с клавиатуры и команды для резца
  while (c == 0) delay(100);  // барьер - пропустит только когда в с придет код нажатой клавиши - через прерывание от клавиатуры
  delay(500);                 // дополнительные полсекунды на регистрирование отпускания нажатой клавиши
  esc = false;                // разрешаем проход по циклам движения резца
  pause = false;              // cнимаем с паузы
  Serial.println(c, HEX);     // отчитываемся в консоль о принятой команде
    // отработка вариантов нажатых клавиш:
    // выбор резца (одинарный / двойной):
  if (c == 0x16) setCutterSolo();           // 1
  if (c == 0x1E) setCutterDuo();            // 2
    // макродвижения резца
  if (c == 0x05) goHomeX();                 // F1
  if (c == 0x06) makeTailLeft();            // F2
  if (c == 0x04) makeNotch92();             // F3
  if (c == 0x0C) makeTailRight();           // F4
    // движения резца в диапазоне одного ребра
  if (c == 0x03) goEdgeLeft();              // F5
  if (c == 0x0B) goHomeY();                 // F6
  if (c == 0x83) goY(depth, true);          // F7
  if (c == 0x0A) goEdgeRight();             // F8
    // микродвижения резца
  if (c == 0x01) goX(currentX - 4, true);   // F9
  if (c == 0x09) goY(currentY - 60, false); // F10
  if (c == 0x78) goY(currentY + 60, true);  // F11
  if (c == 0x07) goX(currentX + 4, true);   // F12
    // движения резца на стандартный шаг
  if (c == 0x6B) goX(currentX - edgeStep, true);   // Arrow Left
  if (c == 0x72) goY(currentY - miniDepth, false); // Arrow Back
  if (c == 0x75) goY(currentY + miniDepth, true);  // Arrow Forward
  if (c == 0x74) goX(currentX + edgeStep, true);   // Arrow Right
    // крупные программы
  if (c == 0x1C) workSolo();                       // A
  if (c == 0x1B) workDuo();                        // S
  if (c == 0x23) workLeftTailOnShiftReverse();     // D
  if (c == 0x2B) makeCornerRight();                // F
    // быстрые движения в левый - правый край
  if (c == 0x6C) goX(homeX, false);                 // Home
  if (c == 0x69) goX(endX, false);                  // End
//  if (c == 0x) ;           //
}

//void test() {
//  for (int i = 0; i < 1000; i++) {
//    Serial.print("endPinLeft: ");
//    Serial.print(isEndPressed(endPinLeft));
//    Serial.print("; endPinForward: ");
//    Serial.print(isEndPressed(endPinForward));
//    Serial.println(";");
//    delay(100);
//  }
//}

void keyPress() {
  if (q > 1 && q < 11)w |= digitalRead(2) << q - 2;
  if (++q > 11) {
    esc = true;               //любая клавиша кроме пробела и Enter - выбрасывает из текущего цикла, если таковой имеется
    c = w;                    //...открывает барьрер в loop для выполнения вновь введенной команды
    q = 1; w = 0;             //...обнуляет буфера ввода с клавиатуры
  }
}

// мигание диодом указанное количество раз
void blinker(int counter) {
  Serial.println("    blinker() - start with - " + String(counter));
  while (counter-- > 0) {
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
  }
  Serial.println("    blinker() - finish"); 
}

// обнуление координат ребер
void resetCutter() {
  Serial.println("  resetCutter() - start");
  for (int i = 0; i < 100; i++)
    edges[i] = homeX;
  Serial.println("  resetCutter() - finish - reseted");
}

// назначение координат ребер для позиционирования правой кромки моно-резца
void setCutterSolo() {
  Serial.println("  setCutterSolo() - start");
  resetCutter();                // обнуление координат ребер
  edges[97] = homeX;            // отрезок 97-98 формирует правую секцию левого хвоста изделия
  edges[98] = 470*4;            // формирует -1-е (широкое) ребро слева
  edges[99] = 500*4;            // формирует -1-е (широкое) ребро справа
  edges[0]  = 510*4;            // нулевой стандартный проход одиночным резцом
  for (int i = 1; i < 92; i++)  // пачка проходов через стандартный интервал
    edges[i] = edges[i-1] + edgeStep;
  edges[92] = 4523*4;            // формирует 92-е (широкое) ребро слева
  edges[93] = 4553*4;            // формирует 92-е (широкое) ребро справа
  edges[94] = 5030*4;            // отрезок 92-93 формирует правый хвост изделия
  blinker(1);                    // мигаем один раз - одинарный резец
  Serial.println("  setCutterSolo() - finish");
  
}

void setCutterDuo() {
  Serial.println("  setCutterDuo() - start");
  resetCutter();                 // обнуление координат ребер
  edges[0] = 544*4;              // нулевой стандартный проход двойным резцом
  for (int i = 1; i <= 90; i++)  // пачка проходов через стандартный интервал
    edges[i] = edges[i-1] + edgeStep;
  for (int i = 91; i <= 94; i++) // 4 холостых дубля после edges[90] - для getEdgeRight()
    edges[i] = edges[i-1];       
  blinker(2);                    // мигаем два раза - двойной резец
  Serial.println("  setCutterDuo() - finish");
}

bool isEndPressed(int endPin) {   // проверяет, нажат ли указаный концевик
  int weight = 0;
  for (int i = 0; i < 10; i++)
      weight += analogRead(endPin);
  bool pressed = weight > 10150;
  return (pressed);
}

void sendPulse(char motor, int delayPulse) {   // один такт указаного мотора с указаной паузой
  Serial.println("    x" + String(currentX) + "." + String(isEndPressed(endPinLeft)) + " y" + String(currentY) + "." + String(isEndPressed(endPinBack)));  
  int pulsePin = 0;
  if (motor == 'X') pulsePin = pinPulseX;
  if (motor == 'Y') pulsePin = pinPulseY;
  digitalWrite(pulsePin, HIGH);
  digitalWrite(pulsePin, LOW);
  delay(delayPulse);
}

void goHomeY() {                                // позиционируем суппорт во фронтальную линию
  Serial.println("  goHomeY() - start from " + String(currentY));
  if (currentY != homeY && !esc) {           // выполняем позиционирование только если не стоим на линии фронта
    digitalWrite(pinDirY, LOW);                 // направление - назад
    while (!isEndPressed(endPinBack) && !esc) { // откатываем суппорт пока не нажат концевик и не esc
      currentY--;
      sendPulse('Y', delayRunY);
    } 
    digitalWrite(pinDirY, HIGH);                // направление - вперед
    while (isEndPressed(endPinBack) && !esc) {   // откатываем суппорт на линию фронта (отпуск концевика)
      currentY++;
      sendPulse('Y', delayRunY);
    }
    currentY = homeY;                        // фиксируем ноль (линию фронта) по отпуску концевика
  }
  Serial.println("  goHomeY() - finish. currentY set to homeY = " + String(currentY));
}

void goHomeX() {                                // позиционируем суппорт в левую домашнюю точку
  Serial.println("  goHomeX() - start from " + String(currentX));
  if (currentY != homeY && !esc) goHomeY();  // выходим на линию фронта (currentY == homeY)
  if (currentX != homeX && !esc) {           // выполняем позиционирование только если не стоим в домашней точке
    digitalWrite(pinDirX, HIGH);                // направление - налево
    while (!isEndPressed(endPinLeft) && !esc) {  // откатываем суппорт пока не нажат концевик и не esc
      currentX--;
      sendPulse('X', delayRunX);
    }
    digitalWrite(pinDirX, LOW);                 // направление - направо
    while (isEndPressed(endPinLeft) && !esc) {   // откатываем суппорт до отпускания концевика
      currentX++;
      sendPulse('X', delayCutX);
    }
    currentX = homeX;                        // фиксируем линию фронта по отпуску концевика
  }
  Serial.println("  goHomeX() - finish. currentX set to homeX = " + String(currentX));
}

void goX(int targetX, bool cut) {
  Serial.println("  goX() - start - from " + String(currentX) + " to " + String(targetX) + " " + String(cut ? " cut" : " run"));
  int distance = targetX - currentX;
  int stepPulse = distance > 0 ? 1 : -1;
  int delayX = cut ? delayCutX : delayRunX;
  digitalWrite(pinDirX, distance < 0 ? HIGH : LOW);
  while (currentX != targetX && !esc) {
     currentX += stepPulse;
     sendPulse('X', delayX);
  }
  Serial.println("  goX() - finish at " + String(currentX));
}

void goY(int targetY, bool cut) {
  Serial.println("  goY() - start - from " + String(currentY) + " to " + String(targetY) + String(cut ? " cut" : " run"));
  int distance = targetY - currentY;
  int stepPulse = distance > 0 ? 1 : -1;
  int delayY = cut ? delayCutY : delayRunY;
  digitalWrite(pinDirY, distance > 0 ? HIGH : LOW);
  while (currentY != targetY && !esc) {
     currentY += stepPulse;
     sendPulse('Y', delayY);
  }
  Serial.println("  goY() - finish at " + String(currentY));
}

void makeTailLeft() {
  Serial.println("makeTailLeft() - start");
  goHomeY();
  setCutterSolo();
  makeNotchWide(edges[97], edges[98] - 10);
  makeNotch(edges[99]);
  goHomeY();
  Serial.println("makeTailLeft() - finish");
}

void makeTailRight() {
  Serial.println("makeTailRight() - start");
  goHomeY();
  setCutterSolo();
  makeNotch(edges[92]);
  makeNotchWide(edges[93] + 10, edges[94]);
  
  goX(edges[93], false);
  makeCornerRight();                   //  оформление правого угла одним проходом

  goHomeY();
  Serial.println("makeTailRight() - finish");
}

void makeCornerRight() {
  goY(depth + miniDepth, true);
  goX(edges[94], true);
  goHomeX();
}

void makeNotch(int positionX) {
  Serial.println("  makeNotch() - start" );
  goHomeY();
  goX(positionX, false);
  goY(depth, true);
  delay(1000);
  goHomeY();
  Serial.println("  makeNotch() - start" );
}

void makeNotch92() {
  Serial.println("makeNotch92() - start");
  goHomeY();
  int edgeCurrent = getEdgeRight();
  while (edgeCurrent <= 91 && !esc)
    makeNotch(edges[edgeCurrent++]);
  goHomeY();
  Serial.println("makeNotch92() - finish");
}

void makeNotchWide(int left, int right) {
  Serial.println("  makeNotchWide() - start - from " + String(left) + " to " + String(right));
  goHomeY();
  goX(left, false);
  while (currentY <= depth - 2*miniDepth && !esc) {
    goY(currentY + miniDepth, true);
    goX(right, true);
    goY(currentY + miniDepth, true);
    goX(left, true);
  }
  goHomeY();
  Serial.println("  makeNotchWide() - finish" );
}

int getEdgeRight() {      // return number of first edge situated to the right from currentX
  Serial.println("      getEdgeRight() - start");
  int edgeRight = 94;
  for (int i = 94; i >= 0; i--)
    if (currentX < edges[i])
      edgeRight = i;
  Serial.println("      getEdgeRight() - finish - return edge " + String(edgeRight) + " on " + String(edges[edgeRight]/4) + "mm");
  return edgeRight;
}

void goEdgeRight() {
  Serial.println("goEdgeRight() - start");
  goHomeY();
  int edgeRight = getEdgeRight();
  goX(edges[edgeRight], false);
  Serial.println("goEdgeRight() - finish");
}

void goEdgeLeft() {
  Serial.println("goEdgeLeft() - start");
  goHomeY();
  int edgeLeft = getEdgeRight();
  if (edgeLeft > 0) edgeLeft--;   // позиция слева - это которая слева от правой
  if (edgeLeft > 0 && edges[edgeLeft] == currentX) edgeLeft--;  // если позиция слева от правой оказалась текущей, то берем следующую слева
  goX(edges[edgeLeft], false);
  Serial.println("goEdgeLeft() - finish");
}

void workSolo(){
  setCutterSolo();
  makeTailLeft();
  makeNotch92();
  makeTailRight();
}

void workDuo() {
  setCutterDuo();
  makeNotch92();
}

void workLeftTailOnShiftReverse() {
  makeNotchWide(4600*4, 5000*4);
}

//----------------EOF------------------------

//дописать угловой проход в левом хвосте
//написать проход направо до позиции 5000*4
