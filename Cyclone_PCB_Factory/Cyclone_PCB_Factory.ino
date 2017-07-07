//------------------------------------------------------------------------------
// LIBRERIAS
//------------------------------------------------------------------------------
#include <Servo.h>

//------------------------------------------------------------------------------
// CONSTANTES SOFTWARE
//------------------------------------------------------------------------------


// BASADO EN
/*
  https://thingiverse-production-new.s3.amazonaws.com/assets/87/b0/2c/f5/4c/CheatSheet.pdf
*/

#define BAUD 115200
#define MAX_BUF 100

#define STEPS_PER_MM_X (float)(12500.0/87.0)
#define STEPS_PER_MM_Y (float)(100000.0/677.0)
//fijar estos 3 parametros de acuerdo a especificaciones maximas del motor y/o del ensamble mecanico
#define MAX_FEEDRATE   650.0
#define MIN_FEEDRATE   50.0
#define DEFAULT_FEEDRATE   400.0
#define MIN_STEP_DELAY 1000000.0 / MAX_FEEDRATE

#define Y_PASO_POR_MM (float)(100000.0/677.0)
#define Y_MM_POR_PASO (float)(677.0/100000.0)
#define X_PASO_POR_MM (float)(12500.0/87.0)
#define X_MM_POR_PASO (float)(87.0/12500.0)

#define Z_UP 15
#define Z_DOWN 0

//Servo Motor en Z
Servo MiServo;

// velocidades globales

float fr =     DEFAULT_FEEDRATE;  // version humano
long  step_delay = 1000000 / DEFAULT_FEEDRATE; // version maquina

// Ubicacion global
float px, py;
// Modo Absoluto o relativo
char mode_abs = 1; // 1 absoluto(default) 0 relativo

//Buffer de comunicacion y variales para recorrerlo
char  buffer[MAX_BUF];  // Almacena una linea entera
int   iterador;            // how much is in the buffer

//Modo Verbose
unsigned int verboseVar = 0; // 1 -> Activo 0 -> Desactivado

//Milimitros (mm) o Pulgadas (in)
unsigned int mm = 1; // 1 -> Modo en mm 0 -> Modo en in

//------------------------------------------------------------------------------
// CONSTANTES HARDWARE
//------------------------------------------------------------------------------

#define M1_STEP 2     /*POR FAVOR CAMBIAR ESTO CUANDO LEA EL CODIGO. NO SEA AFANADO*/
#define M1_DIR  5     /*POR FAVOR CAMBIAR ESTO CUANDO LEA EL CODIGO. NO SEA AFANADO*/
#define M1_ENA  8     /*POR FAVOR CAMBIAR ESTO CUANDO LEA EL CODIGO. NO SEA AFANADO*/

#define M2_STEP 3     /*POR FAVOR CAMBIAR ESTO CUANDO LEA EL CODIGO. NO SEA AFANADO*/
#define M2_DIR  6     /*POR FAVOR CAMBIAR ESTO CUANDO LEA EL CODIGO. NO SEA AFANADO*/
#define M2_ENA  8     /*POR FAVOR CAMBIAR ESTO CUANDO LEA EL CODIGO. NO SEA AFANADO*/

#define ENDSTOP_X 11
#define ENDSTOP_Y 10

#define AXIS_Z 9

//------------------------------------------------------------------------------
// METODOS HARDWARE
//------------------------------------------------------------------------------

void m1step(int dir) {
  digitalWrite(M1_ENA, LOW);
  digitalWrite(M1_DIR, dir);
  digitalWrite(M1_STEP, HIGH);
  digitalWrite(M1_STEP, LOW);
}

void m2step(int dir) {
  digitalWrite(M2_ENA, LOW);
  digitalWrite(M2_DIR, dir);
  digitalWrite(M2_STEP, HIGH);
  digitalWrite(M2_STEP, LOW);
}

void zAxisMove(float dir) {
  if (dir > 0) {
    MiServo.write(Z_UP);
    delay(100);
    if (verboseVar == 1)
      Serial.println(F("Motor Z Movido arriba."));
  }
  else {
    if (dir < 0) {
      MiServo.write(Z_DOWN);
      delay(100);
      if (verboseVar == 1)
        Serial.println(F("Motor Z Movido abajo."));
    }
  }
}

void disable() {
  digitalWrite(M1_ENA, HIGH);
  digitalWrite(M2_ENA, HIGH);
  if (verboseVar == 1)
    Serial.println(F("Motores Desactivados."));
}

void enable() {
  digitalWrite(M1_ENA, LOW);
  digitalWrite(M2_ENA, LOW);
  if (verboseVar == 1)
    Serial.println(F("Motores Activados."));
}

void setup_controller() {
  pinMode(M1_ENA, OUTPUT);
  pinMode(M2_ENA, OUTPUT);
  pinMode(M1_STEP, OUTPUT);
  pinMode(M2_STEP, OUTPUT);
  pinMode(M1_DIR, OUTPUT);
  pinMode(M2_DIR, OUTPUT);
  pinMode(ENDSTOP_X, INPUT_PULLUP);
  pinMode(ENDSTOP_Y, INPUT_PULLUP);
  MiServo.attach(9);
  MiServo.write(Z_UP);
}





//------------------------------------------------------------------------------
// METODOS SOFTWARE
//------------------------------------------------------------------------------

void help() {
  Serial.println(F("Este es un ejemplo de funcion de ayuda: \n"));
  Serial.println(F("Comandos soportados:"));
  Serial.println(F("G00 [X(steps)] [Y(steps)] [F(feedrate)]; - fast line"));
  Serial.println(F("G01 [X(steps)] [Y(steps)] [F(feedrate)]; - fast line igual que G00 "));
  Serial.println(F("G04 P[seconds]; - delay"));
  Serial.println(F("G20 Cambio a pulgadas (in)."));
  Serial.println(F("G21 Cambio a milimetros (mm)."));
  Serial.println(F("G28; -Volver a home"));
  Serial.println(F("G90; - Modo absoluto"));
  Serial.println(F("G91; - Modo relativo"));
  Serial.println(F("G92 [X(steps)] [Y(steps)]; - Cambiar setpoint"));
  Serial.println(F("M2; -Codigo G finalizado"));
  Serial.println(F("M17; -Habilitar motores"));
  Serial.println(F("M18; - Deshabilitar motores"));
  Serial.println(F("M100; - Imprimir ayuda"));
  Serial.println(F("M111 S[Debug Level]; -Unicamente activado desactivado"));
  Serial.println(F("M112; -Parada de emergencia"));
  Serial.println(F("M114; - reporte de estado de la maquina"));
  Serial.println(F("M115; Obtener informacion del software Firmeware"));
  Serial.println(F("NOTA: TODOS LOS COMANDOS DEBEN TERMINAR CON NUEVA LINEA O -ESPACIO#-"));
}



// la funcion delayMicroseconds de arduino no funciona para valores mayores a 16k. por eso toca usar esta.
void pause(long us) {
  delay(us / 1000);
  delayMicroseconds(us % 1000);
}





void feedrate(float nfr) {
  if (fr == nfr) return;


  if (nfr > MAX_FEEDRATE || nfr < MIN_FEEDRATE) { // depurar
    Serial.print(F("WARNING: Feedrate debe ser mayor a  "));
    Serial.print(MIN_FEEDRATE);
    Serial.print(F("pero menor a  "));
    Serial.print(MAX_FEEDRATE);
    Serial.println(F("pasos/ssegundo. NO SE CONGIFURA NADA"));
    return;//salir sin cambiar el valor
  }
  step_delay = 1000000.0 / nfr; //valor maquina
  fr = nfr;//valor humano
}

/**
  pos actual, util para tener pos absolutas y relativas.
**/
void position(float npx, float npy) {
  px = npx;
  py = npy;
}


/**
  Usa el algoritmo de  bresenham para delinear lineas en dos motores XY
  https://es.wikipedia.org/wiki/Algoritmo_de_Bresenham
**/

void lineaBresenhamPasos(float newx, float newy) {
  long i;
  long over = 0;

  long dx  = newx - px;
  long dy  = newy - py;
  int dirx = dx > 0 ? LOW : HIGH;
  int diry = dy > 0 ? HIGH : LOW; //diry = dy>0?LOW:HIGH; dependiendo de ensamble POR FAVOR revisar.
  dx = abs(dx);
  dy = abs(dy);

  if (dx > dy) {
    over = dx / 2; //OJO division entera
    for (i = 0; i < dx; ++i) {
      m1step(dirx);
      over += dy;
      if (over >= dx) {
        over -= dx;
        m2step(diry);
      }
      pause(step_delay);
    }

  } else {/*dx<=dy*/
    over = dy / 2;
    for (i = 0; i < dy; ++i) {
      m2step(diry);
      over += dx;
      if (over >= dy) {
        over -= dy;
        m1step(dirx);
      }
      pause(step_delay);
    }
  }
  px = newx;
  py = newy;
}



/**
  Busca el caracter /code/ en la variable  buffer global,  y  retorna el flotante que encuentre a continuacion
  Si no se encuentra nada se retorna /val/
**/
float parsenumber(char code, float val) {
  char *ptr = &buffer[0];
  while (ptr != NULL && *ptr && ptr < buffer + iterador) {
    if (*ptr == code) {
      return atof(ptr + 1);
    }
    ptr++;
  }
  return val;
}


/**
  Escribe en el serial el codigo y el valor decifrado, útil para hacer depuracion
*/
void output(const char *code, float val) {
  Serial.print(code);
  Serial.println(val);
}


/**
  Imprime la configuracion actual.
*/
void where() {
  output("X_mm ", px / STEPS_PER_MM_X);
  output("Y_mm", py / STEPS_PER_MM_Y);
  output("X_ST", px);
  output("Y_ST", py);
  output("F", fr);
  Serial.println(mode_abs ? "MODO ABS" : "MODO REL");
  Serial.println(mm ? "Milimetros" : "Pulgadas");
}
/*
  Home: Movimiento de los ejes X y Y hacia el cero cero de la maquina
*/
void homes () {

  zAxisMove(1);

  while (digitalRead(ENDSTOP_X) == HIGH) {
    m1step(HIGH);
    pause(5000);
  }
  while (digitalRead(ENDSTOP_Y) == HIGH) {
    m2step(LOW);
    pause(5000);
  }

  pause(100000);

  for (int i = 0; i < 300; i++) {
    m1step(LOW);
    pause(10000);
    m2step(HIGH);
    pause(10000);
  }

  while (digitalRead(ENDSTOP_X) == HIGH) {
    m1step(HIGH);
    pause(20000);
  }
  if (verboseVar == 1)
    Serial.print(F("FIN CALIBRACION EJE X...\n"));
  while (digitalRead(ENDSTOP_Y) == HIGH) {
    m2step(LOW);
    pause(20000);
  }
  if (verboseVar == 1)
    Serial.print(F("FIN CALIBRACION EJE Y...\n"));

  for (int i = 0; i < 300; i++) {
    m1step(LOW);
    pause(10000);
    m2step(HIGH);
    pause(10000);
  }

  disable();
  px = 0;
  py = 0;
}

/*
   Firmeware version
*/
void firmeware() {
  Serial.print(F("-------------VERSION firmeware-----------------\n"));
  Serial.print(F("-> Version del software: 1.30\n"));
  Serial.print(F("Realizado por: \n"));
  Serial.print(F("Francisco Calderon.\n"));
  Serial.print(F("Modificado por: \n"));
  Serial.print(F("Alejandro Castro, Oscar Mendez y Daniel Pineda.\n"));
  Serial.print(F("Diseno de la maquina fisica:\n"));
  Serial.print(F(" http://reprap.org/wiki/Cyclone_PCB_Factory\n"));
  Serial.print(F("----------------------------------------------------\n"));
}

/**
   borra el buffer de entrada y envia el caracter de nueva linea de comandos.
*/
void ready() {
  iterador = 0; // reinicia el buffer de entrada
  Serial.print(F(">"));  // envia el caracter > para que se envie nueva linea
}


/**
  Lee el buffer de entrada y reconoce los comandos de haberlos.
  NOTA: solo soporta un comando G o M por linea.
*/
void processCommand() {
  float XX, YY, ZZ;
  int cmd = parsenumber('G', -1);
  switch (cmd) {
    case -1:
      Serial.println(F("No encuentro codigo G en linea..."));
      break;
    case  0:
    case  1:
      Serial.print(F("Comando G"));
      Serial.println(cmd);
      feedrate(parsenumber('F', fr));
      // no olvidar que px y py estan en pasos y los comandos llegan en mm
      if (verboseVar == 1)
        where();
      XX = (parsenumber('X', (mode_abs ? px / STEPS_PER_MM_X : 0)) + (mode_abs ? 0 : px / STEPS_PER_MM_X));
      YY = (parsenumber('Y', (mode_abs ? py / STEPS_PER_MM_Y : 0)) + (mode_abs ? 0 : py / STEPS_PER_MM_Y));
      ZZ = (parsenumber('Z', 0));
      if (verboseVar == 1) {
        Serial.print(F("Moviendo a X= "));
        Serial.print(-X_MM_POR_PASO * px + XX);
        Serial.print(F(", Y= "));
        Serial.print(-Y_MM_POR_PASO * py + YY);
        Serial.print(F("Moviendo Z="));
        if (ZZ <= 0)
          Serial.print(F("ABAJO"));
        else
          Serial.print(F("ARRIBA"));
        Serial.print(F(", F= "));
        Serial.println(fr);
      }
      if (ZZ != 0)
        zAxisMove(ZZ);
      if (mm == 1)
        lineaBresenhamPasos( STEPS_PER_MM_X * XX , STEPS_PER_MM_Y * YY );
      else
        lineaBresenhamPasos( STEPS_PER_MM_X * (XX * 25.4), STEPS_PER_MM_Y * (YY * 25.4));
      if (mode_abs == 0)
        position( parsenumber('X', 0),
                  parsenumber('Y', 0) );
      if (verboseVar == 1)
        where();
      break;
    case  4:
      Serial.print(F("Comando G"));
      Serial.println(cmd);
      pause(parsenumber('P', 0) * 1000);
      Serial.println(F("Pausa realizada..."));
      break;  // dwell
    case 20: //a in
      mm = 0;
      Serial.println(F("Cambio a pulgadas."));
      break;
    case 21://a mm
      mm = 1;
      Serial.println(F("Cambio a mm."));
      break;
    case  28:
      Serial.print(F("Comando G"));
      Serial.println(cmd);
      homes();
      break;  // dwell
    case 90:  //modo absoluto
      Serial.print(F("Comando G"));
      Serial.println(cmd);
      Serial.println(F("modo absoluto"));
      mode_abs = 1;
      break;
    case 91:  //modo relativo
      Serial.print(F("Comando G"));
      Serial.println(cmd);
      Serial.println(F("modo relativo"));
      mode_abs = 0;
      break;
    case 92:  // definir pos
      Serial.print(F("Comando G"));
      Serial.println(cmd);
      position( parsenumber('X', 0),
                parsenumber('Y', 0) );
      Serial.println(F("Posicion en cero cero"));
      break;
    default:
      Serial.print(F("Comando G"));
      Serial.print(cmd);
      Serial.println(F(" No implementado..."));

      break;
  }

  cmd = parsenumber('M', -1);
  switch (cmd) {
    case -1:
      Serial.println(F("Sin codigo M en linea..."));
      break;
    case 2:
      Serial.println(F("Codigo G finalizado"));
      zAxisMove(1);
      break;
    case 17:  // enable motors
      Serial.print(F("Comando M"));
      Serial.println(cmd);
      enable();
      Serial.println(F("Motores habilitados"));
      break;
    case 18:  // disable motors
      Serial.print(F("Comando M"));
      Serial.println(cmd);
      disable();
      Serial.println(F("Motores desahabilitados"));
      break;
    case 100:
      Serial.print(F("Comando M"));
      Serial.println(cmd);
      help();
      break;
    case 111:  // debug level
      Serial.print(F("Comando M"));
      Serial.println(cmd);
      if (parsenumber('S', -1) == 1) {
        verboseVar = 1;
        Serial.println(F("Modo Verbose Activado"));
      }
      else {
        if ((parsenumber('S', -1) == 0)) {
          verboseVar = 0;
          Serial.println(F("Modo Verbose Desactivado"));
        }
        else
          Serial.println(F("Opcion invalida. Ejecucion: M111 S[Debug Level]"));
      }
      break;
    case 112:  // Emergency Stop
      Serial.print(F("Comando M"));
      Serial.println(cmd);
      zAxisMove(1);
      disable();
      Serial.println(F("Parada de emergencia."));
      break;
    case 114:
      Serial.print(F("Comando M"));
      Serial.println(cmd);
      where();
      break;
    case 115:
      Serial.print(F("Comando M"));
      Serial.println(cmd);
      firmeware();
      break;
    default:
      Serial.print(F("Comando M"));
      Serial.print(cmd);
      Serial.println(F(" No implementado..."));
      break;
  }
}



/**
   Funcion setup arduino
*/
void setup() {
  Serial.begin(BAUD);
  setup_controller();
  position(0, 0); // pos de inicio
  feedrate(DEFAULT_FEEDRATE);  // inicializa con velocidad por defecto en caso de que no se provea ninguna
  help();  // say hello
  ready();
}

/**
   Funcion loop de arduino
*/
void loop() {
  while (Serial.available() > 0) { // Si algo esta disponible
    char c = Serial.read(); // lea
    Serial.print(c);  // echo
    if (iterador < MAX_BUF - 1)
      buffer[iterador++] = c; // guarde y aumente iterador
    if ((c == '\n') || (c == '\r') || (c == '#') )
    { // si se recibe una linea entera
      buffer[iterador] = 0; // finalice el buffer con 0
      Serial.print(F("--\r\n"));  // echo de un caracter de return y nueva linea
      processCommand();  // procese el comando
      ready(); // pida nuevo comando.
    }
  }
}



