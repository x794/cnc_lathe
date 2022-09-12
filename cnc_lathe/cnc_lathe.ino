volatile uint8_t q = 1, w = 0, c = 0; //  q, w - для обработки прерывания с клавиатуры, c - передает в loop команду от клавиатуры
volatile bool esc = false;   // флаг выброса из штатных проходов резца. Выбрасывает по true. Сменяется на true при нажатии любой клавиши кроме Пробел и Enter
volatile bool pause = false; // флаг заморозки штатных проходов резца. Замораживается по true. Сменяется на true только при нажатии пробела, обратно - по Enter
int pulsesInTenthX = 4;      // количество тактов для продольного сдвига суппорта на 1 десятку
int pulsesInTenthY = 20;     // количество тактов для поперечного сдвига суппорта на 1 десятку
int latheMove = 10;          // величина снимаемого во время точения слоя. В десятках
int latheLag = 100;          // время съема слоя во время точения. Временной разделитель шагов точения
int pulseDelay = 2;          // задержка между импульсами на драйвер мотора

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
  //функция keyPress вызывается при нисходящем срабатывании сигнала на D3
  attachInterrupt(1, keyPress, FALLING);
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
  if (c == 0x1C) walk('L', 44);    //  A
  if (c == 0x23) walk('R', 44);    //  D
  if (c == 0x6C) walk('L', 5200);  //  Home
  if (c == 0x69) walk('R', 5200);  //  End
  if (c == 0x1D) walk('F', 132);   //  W
  if (c == 0x1B) walk('B', 132);   //  S
    //микродвижения резца
  if (c == 0x6B) walk('L', latheMove);    //   Arrow_Left
  if (c == 0x74) walk('R', latheMove);    //  Arrow_Right
  if (c == 0x75) walk('F', latheMove);    //     Arrow_Up
  if (c == 0x72) walk('B', latheMove);    //   Arrow_Down
}

void keyPress() {
  if (q > 1 && q < 11)w |= digitalRead(2) << q - 2;
  if (++q > 11) {
//    if (c == 0x29) { pause = true; digitalWrite(LED_BUILTIN, HIGH);  }  //Пробел - ставит резец на паузу и запитывает диод
//    if (c == 0x5A) { pause = false; digitalWrite(LED_BUILTIN, LOW);  }  //Enter  - снимает резец с паузы и  отключает диод
    esc = true;       //любая клавиша кроме пробела и Enter - выбрасывает из текущего цикла, если таковой имеется
    c = w;            //...открывает барьрер в loop для выполнения вновь введенной команды
    q = 1; w = 0;     //...обнуляет буфера ввода с клавиатуры
  }
}

bool stoper(int endPin) {                 // указаный концевик нажат?
  int weight = 0;
  for (int i = 0; i < 20; i++)
    if (digitalRead(endPin) == HIGH)
      weight++;
  
  Serial.println(weight);
  if (weight < 14)
    return false;
  else
    return true;
}

void pulse(char axis) {                    // один такт мотора в указаном направлении
  int pulsePin = 0;
  switch (axis) {
    case 'X':
      pulsePin = xPulsePin;
      break;
    case 'Y':
      pulsePin = yPulsePin;
      break;
  }  
  digitalWrite(pulsePin, HIGH);
  digitalWrite(pulsePin, LOW);
  Serial.println(pulsePin);
  delay(pulseDelay);
}

void tenth(char axis) {          // сдвиг суппорта на одну десятку в указаном направлении
  int pulsesInTenth = 0;
  switch (axis) {
    case 'X':
      pulsesInTenth = pulsesInTenthX;
      break;
    case 'Y':
      pulsesInTenth = pulsesInTenthY;
      break;
  }  

  for (int i = 0; i < pulsesInTenth; i++) {
    pulse(axis);
  }
}

void walk(char dir, int tenths) {        // сдвиг суппорта в направлении dir на tenth десяток. Для холостого хода и для микродвижений точения
  
  int endPin = 0;
  char axis = 'O';
  
  switch (dir) {
    case 'L':
      endPin = leftEndPin;
      axis = 'X';
      digitalWrite(xDirPin, HIGH);
      break;
    case 'R':
      endPin = rightEndPin;
      axis = 'X';
      digitalWrite(xDirPin, LOW);
      break;
    case 'F':
      endPin = forwardEndPin;
      axis = 'Y';
      digitalWrite(yDirPin, HIGH);
      break;
    case 'B':
      endPin = backEndPin;
      axis = 'Y';
      digitalWrite(yDirPin, LOW);
      break;
  }
  
  Serial.println(dir);
  Serial.println(tenths);
  Serial.println(endPin);
  Serial.println(axis);


  while(!stoper(endPin) && !esc && tenths-- > 0)    // движемся порциями до стопера, нажатия esc или окончания пути
    tenth(axis);

  while(stoper(endPin) && !esc) { // если нажат левый концевик - импульсами вправо с задержками - до отпускания или до esc
    digitalWrite(xDirPin, HIGH);
    pulse(axis);
    delay(20);
  }
}

void slice(char dir, int tenths) {      // реализует движение суппорта порциями, через walk 
  while (tenths > latheMove) {          // пока остаток движения больше порции - движемся на порцию
    walk(dir, latheMove);
    tenths -= latheMove;
    delay(latheLag);
  }
  walk(dir, tenths);                    // остаточное движение суппорта (которое <= порции)
}

//void latheStep(char stepType) {       // проточка - выход - сдвиг вправо на величину в зависимости от типа шага
//  int right = 0;
//  switch (stepType) {   // определяем тип шага по входному параметру
//    case 'A':
//      right = 15;       // шаг сквозной проточки
//      break;
//    case 'B':
//      right = 54;       // шаг через толстое ребро
//      break;
//    case 'C':
//      right = 44;       // шаг стандартного ребра
//      break;
//  }
//    slice('F', 132);    // прерывистое погружение резца в тело
//    walk('B', 132);     // вывод резца из тела
//    walk('R', right);   // сдвиг вправо на соответствующее типу шага расстояние
//}
