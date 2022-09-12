volatile uint8_t q = 1, w = 0, c = 0; //  q, w - для обработки прерывания с клавиатуры, c - передает в loop команду от клавиатуры
volatile bool esc = false;    //  флаг выброса из штатных проходов резца. Выбрасывает по true. Сменяется на true при нажатии любой клавиши кроме Пробел и Enter
volatile bool pause = false;  //  флаг заморозки штатных проходов резца. Замораживается по true. Сменяется на true только при нажатии пробела, обратно - по Enter
int pulsesInTenthX = 50;             //  количество тактов для продольного сдвига суппорта на 1 десятку
int pulsesInTenthY = 20;             //  количество тактов для поперечного сдвига суппорта на 1 десятку
volatile int leftEndCounter = 0;     //  счетчик срабатываний концевика слева (для устранения последствий дрожания сигнала от концевика)
volatile int rightEndCounter = 0;    //  счетчик срабатываний концевика справа
volatile int forwardEndCounter = 0;  //  счетчик срабатываний концевика погружения
volatile int backEndCounter = 0;     //  счетчик срабатываний концевика отката

//назначение пинов концевикам
byte leftEndPin = 5;
byte rightEndPin = 6;
byte forwardEndPin = 7;
byte backEndPin = 8;
//назначение пинов моторам
byte xPulsePin = 9;
byte yPulsePin = 10;
byte xDirPin = 11;
byte yDirPin = 12;

void setup() {
  Serial.begin(57600);
  Serial.println(11);
  //инициация встроенного диода для теста клавиатуры
  pinMode(LED_BUILTIN, OUTPUT);
  //инициация пинов на прием сигнала от концевиков (4 штуки)
  pinMode(5, INPUT);            
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  //инициация пинов на передачу управляющих команд моторам (4 штуки)
  pinMode(9, OUTPUT);           
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  //инициация прерывания 1 - всегда ожидается на D3 (прерывания 0 - на D2)),
  //функция ps2Keyboard вызывается при нисходящем срабатывании сигнала на D3
  attachInterrupt(1, ps2Keyboard, FALLING);
}

void loop() {                 //вход в основной рабочий цикл
  c = 0;  q = 1; w = 0;       //обнуление буферов для ввода с клавиатуры и команды для резца
  while (c == 0) delay(100);  //барьер - пропустит только когда в с придет код нажатой клавиши - через прерывание от клавиатуры
  delay(500);                 //дополнительные полсекунды на регистрирование отпускания нажатой клавиши
  esc = false;                //разрешаем проход по циклам движения резца
  pause = false;              //cнимаем с паузы
  Serial.println(c, HEX);     //отчитываемся в консоль о принятой команде
  //отработка вариантов нажатых клавиш:
    //тест клавиатуры:
  if (c == 0x69) digitalWrite(LED_BUILTIN, HIGH); //1 на цифровом блоке
  if (c == 0x7A) digitalWrite(LED_BUILTIN, LOW);  //3 на цифровом блоке
    //штатные движения резца - полный шаг влево-вправо (на 4,4 мм), погружение резца (на 13 мм), откат резца (на 13 мм)
  if (c == 0x1C) moveLeft(800, 1);    //  A
  if (c == 0x23) moveRight(800, 1);   //  D
  if (c == 0x1D) moveForward(2220, 1);//  W
  if (c == 0x1B) moveBack(2220, 1);   //  S
    //микродвижения резца
  if (c == 0x6B) moveLeft(100, 1);    //  Arrow_Left
  if (c == 0x74) moveRight(100, 1);   //  Arrow_Right
  if (c == 0x75) moveForward(222, 1); //  Arrow_Up
  if (c == 0x72) moveBack(222, 1);    //  Arrow_Down
}

void ps2Keyboard() {
  if (q > 1 && q < 11)w |= digitalRead(2) << q - 2;
  if (++q > 11) {
//    if (c == 0x29) { pause = true; digitalWrite(LED_BUILTIN, HIGH);  }  //Пробел - ставит резец на паузу и запитывает диод
//    if (c == 0x5A) { pause = false; digitalWrite(LED_BUILTIN, LOW);  }  //Enter  - снимает резец с паузы и  отключает диод
    esc = true;       //любая клавиша кроме пробела и Enter - выбрасывает из текущего цикла, если таковой имеется
    c = w;            //...открывает барьрер в loop для выполнения вновь введенной команды
    q = 1; w = 0;     //...обнуляет буфера ввода с клавиатуры
  }
}

bool stopper(int endPin) {                 // возвращает true, если нажат концевик. Принимает пин интересующего концевика
  int counter = 0;
  for (int i = 0; i < 20; i++)
    if (digitalRead(endPin) == HIGH)
      counter++;
  
  if (counter < 11)
    return false;
  else
    return true;
}

void pulseX() {                           // один такт мотора X, без направления
  if (esc == false && pause == false) {
    digitalWrite(xPulsePin, HIGH);
    digitalWrite(xPulsePin, LOW);
    delay(1);
  }
}

void tenthX() {                           // сдвиг по X на одну десятку, без направления
  for (int i = 0; i < pulsesInTenthX; i++) {
    pulseX();
  }
}

void moveX(int tenths, byte dir) {        // сдвиг по X на tenth десяток в направлении dir
  if (dir == false) {                     // если направление - налево, то сдвигаем влево на tenths десяток, но пока не нажат левый концевик
    digitalWrite(xDirPin, LOW);
    while(tenths-- > 0 && !stoper(leftEndPin))
      tenthX();
  } else {                                // иначе, сдвигаем вправо на tenths десяток, но пока не нажат левый концевик
    digitalWrite(xDirPin, HIGH);
    while(tenths-- > 0 && !stoper(rightEndPin))
      tenthX();
  }
}

void moveBack(int m, byte d) {
  digitalWrite(yDirPin, HIGH); //направление мотора Y - извлечение
  while(m > 0 && backEndCounter < 15) { //извлекаем на m минут, но пока не нажат esc и концевик отката
    moveMinuteY();
    delay(d);
    m--;
  }
}void movePulseY() {
  if (esc == false && pause == false) {
    digitalWrite(yPulsePin, HIGH);
    digitalWrite(yPulsePin, LOW);
    delay(1);
  }
}

void moveTenthY() {
  forwardEndCounter = 0;
  backEndCounter = 0;
  for (int i = 0; i < pulsesInTenthY; i++) {
    movePulseY();
    if (digitalRead(forwardEndPin) == true) forwardEndCounter++;
    if (digitalRead(backEndPin) == true) backEndCounter++;
  }
  Serial.println(digitalRead(forwardEndCounter));
  Serial.println(digitalRead(backEndCounter));
}

void moveForward(int m, byte d) {
  digitalWrite(yDirPin, LOW); //направление мотора Y - погружение
  while(m > 0 && forwardEndCounter < 15) { //погружаем на m минут (тактами по 5 минут с паузой m), но пока не нажат esc и концевик погружения
    for (int i = 0; i < min(5, m); i++)
      moveTenthY();
    delay(d);
    m-=5;
  }
}

void moveBack(int m, byte d) {
  digitalWrite(yDirPin, HIGH); //направление мотора Y - извлечение
  while(m > 0 && backEndCounter < 15) { //извлекаем на m минут, но пока не нажат esc и концевик отката
    moveMinuteY();
    delay(d);
    m--;
  }
}


//void moveLeft(int h, byte v) {
//  digitalWrite(xDirPin, LOW); //направление мотора X - налево
//  while(h > 0 && esc == false && (digitalRead(leftEndPin) == LOW)) { //тактируем налево h раз, но пока не нажат esc и левый концевик
//    digitalWrite(xPulsePin, HIGH);
//    digitalWrite(xPulsePin, LOW);
//    delay(v);
//    h--;
//  }
//}
//
//void moveRight(int h, byte v) {
//  digitalWrite(xDirPin, HIGH); //направление мотора X - направо
//  while(h > 0 && esc == false && (digitalRead(rightEndPin) == LOW)) { //тактируем направо h раз, но пока не нажат esc и правый концевик
//    digitalWrite(xPulsePin, HIGH);
//    digitalWrite(xPulsePin, LOW);
//    delay(v);
//    h--;
//  }
//}
//
//void movePulseY() {
//  if (esc == false && pause == false) {
//    digitalWrite(yPulsePin, HIGH);
//    digitalWrite(yPulsePin, LOW);
//    delay(1);
//  }
//}
//
//void moveTenthY() {
//  forwardEndCounter = 0;
//  backEndCounter = 0;
//  for (int i = 0; i < pulsesInTenthY; i++) {
//    movePulseY();
//    if (digitalRead(forwardEndPin) == true) forwardEndCounter++;
//    if (digitalRead(backEndPin) == true) backEndCounter++;
//  }
//  Serial.println(digitalRead(forwardEndCounter));
//  Serial.println(digitalRead(backEndCounter));
//}
//
//void moveForward(int m, byte d) {
//  digitalWrite(yDirPin, LOW); //направление мотора Y - погружение
//  while(m > 0 && forwardEndCounter < 15) { //погружаем на m минут (тактами по 5 минут с паузой m), но пока не нажат esc и концевик погружения
//    for (int i = 0; i < min(5, m); i++)
//      moveTenthY();
//    delay(d);
//    m-=5;
//  }
//}
//
//void moveBack(int m, byte d) {
//  digitalWrite(yDirPin, HIGH); //направление мотора Y - извлечение
//  while(m > 0 && backEndCounter < 15) { //извлекаем на m минут, но пока не нажат esc и концевик отката
//    moveMinuteY();
//    delay(d);
//    m--;
//  }
//}

void moveTestY() {
  for(int i = 0; i < 1000; i++) {
    Serial.println("moveForward");
    moveMinuteY();
  }
}
