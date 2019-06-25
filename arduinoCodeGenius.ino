#define PLAYER_WAIT_TIME 5000 // Tempo que o usuário tem para pressionar o botão (5s)

byte lightSequence[100]; // Sequencia de LEDS que vão acendendo
byte sequenceLength = 0; // Tamanho da sequencia atual
byte inputCount = 0; // Número de botões pressionados pelo usuário
byte lastInput = 0; // Último botão pressionado pelo usuário
byte correctCurrentLed = 0; // LED atual correto (que deveria ser pressionado pelo usuário)
bool buttonPressed = false; // Botão pressionado?
bool waitingUserInput = false; // Programa está esperando pelo usuário apertar um botão?
bool resetGame = false; // Indica quando o jogo deve ser reiniciado, pois o usuário perdeu

byte numberPins = 4; // Número de pins = número de LEDS
byte pins[] = {2, 13, 10, 8}; // Pins no Arduino
                              
long inputTime = 0; // variável que armazena o momento em que o usuário aperta o botão

long lastDebounceTime = 0; // Última vez q o output foi acionado
long debounceDelay = 50; // Tempo de debounce
int buttonState = LOW; // estado inicial do botão
bool ledState = false; // Estado do LED (ON = true // OFF = false)

void setup() {
  delay(3000);
  Serial.begin(9600);
  resetGameVariables();
}

// --------- Função que define se os pins (leds) devem se portar como INPUT ou OUTPUT ---------
void setPinMode(byte pin_mode) {
  for(byte i = 0; i < numberPins; i++){
    pinMode(pins[i], pin_mode); 
  }
}

// --------- Função que "escreve" no pin (led) HIGH (com as nossas configurações 3.3V) ou LOW (groud) ---------
void setLedState(byte stateLED) {
  for (byte i = 0; i < numberPins; i++) {
    digitalWrite(pins[i], stateLED); 
  }
}

// --------- Função que faz os leds piscarem várias vezes (numBlinks) por um tempo = 'time' ---------
void blinkLeds(short blinkTime){
  setPinMode(OUTPUT);
  for (int i = 0; i < 5; i++) {
    setLedState(HIGH);
    delay(blinkTime);
    setLedState(LOW);
    delay(blinkTime);
  }
}

// --------- Função que "reseta" todas as variáveis, a fim de começar um novo jogo ---------
void resetGameVariables() {
  blinkLeds(500);
  sequenceLength = 0;
  inputCount = 0;
  lastInput = 0;
  correctCurrentLed = 0;
  buttonPressed = false;
  waitingUserInput = false;
  resetGame = false;
}

// --------- Função para piscar os LEDs ao final do jogo ---------
void EndGameBlink() {
  blinkLeds(50);  
}

// --------- Função que exibe a sequência de leds ---------
void showLedSequence() {
  for (int i = 0; i < sequenceLength; i++) {
      Serial.print("Seq: ");
      Serial.print(i);
      Serial.print("Pin: ");
      Serial.println(lightSequence[i]);
      digitalWrite(lightSequence[i], HIGH);
      delay(500);
      digitalWrite(lightSequence[i], LOW);
      delay(250);
    } 
}

// --------- Função de "Debouce" -> Retorna HIGH ou LOW ---------
int debouceForDigitalRead(byte pin) {
  buttonState = digitalRead(pin); // estado do botão (pressionado ou não)

  if ((millis() - lastDebounceTime) > debounceDelay) { // Buffer de tempo para filtrar interferências

    if ((buttonState == HIGH) && (ledState == false) ) { // Se o botão foi pressionado

      // digitalWrite(pin, HIGH); // Liga-se o LED
      ledState = !ledState; // Troca-se o estado do LED
      lastDebounceTime = millis();
    }
    else if ((buttonState == HIGH) && (ledState == true) ) {

      // digitalWrite(pin, LOW); // Desliga-se o LED
      ledState = -ledState; // Troca-se o estado do LED
      lastDebounceTime = millis();
    }

  }
  return buttonState;
}

// --------- Função que mostra visualmente (pelos LEDS) que o usuário perdeu e reinicia o jogo ---------
void playerLostGame() {
  EndGameBlink();
  delay(1000);
  showLedSequence(); // mostra a sequência correta (a seuência na qual o usuário errou/perdeu)
  delay(1000);
  resetGameVariables();
}

void loop() {  
  if (!waitingUserInput) {
    setPinMode(OUTPUT);
    
    randomSeed(analogRead(A0)); // https://www.arduino.cc/en/Reference/RandomSeed
    lightSequence[sequenceLength] = pins[random(0, numberPins)]; // Novo valor "aleatório" é adicionado à sequência
    sequenceLength++;
    
    showLedSequence();
    
    waitingUserInput = true; // após mostrar a sequência nova, espera-se que o usuário a repita
    inputTime = millis();
  }
  else { 
    setPinMode(INPUT);

    if (millis() - inputTime > PLAYER_WAIT_TIME) { // Se o usuário levou mais tempo que o permitido, ele perde o jogo
      playerLostGame();
      return;
    }      
        
    if (!buttonPressed) {
      correctCurrentLed = lightSequence[inputCount]; // Valor da sequência que o usuário deveria ter pressionado
      Serial.print("Expected: ");
      Serial.println(correctCurrentLed);
      
      for (int i = 0; i < numberPins; i++) {
        if (pins[i] == correctCurrentLed) // Ignora o botão correto (só é preciso verificar se o usuário errou, se não o jogo continua normalmente)
          continue;
        if (debouceForDigitalRead(pins[i]) == HIGH) { // Se o botão (errado) está pressionado, usuário perdeu
          lastInput = pins[i];
          resetGame = true;
          buttonPressed = true; // Previne loops não intencionais
          Serial.print("Read: ");
          Serial.println(lastInput);
        }
      }      
    }

    if (debouceForDigitalRead(correctCurrentLed) == 1 && !buttonPressed) { // Usuário pressionou o botão correto
      inputTime = millis();
      lastInput = correctCurrentLed;
      inputCount++;
      buttonPressed = true; // Previne loops não intencionais
      Serial.print("Read: ");
      Serial.println(lastInput);
    }
    else {
      if (buttonPressed && debouceForDigitalRead(lastInput) == LOW) {  // Usário soltou o botão (depois de pressionar)?
        buttonPressed = false;
        delay(20);
        if(resetGame){
          playerLostGame();
        }
        else {
          if (inputCount == sequenceLength) { // Se o usuário atingiu o limite da sequência (100)
            waitingUserInput = false;
            inputCount = 0;
            delay(1500);
          }
        }
      }
    }    
  }
}