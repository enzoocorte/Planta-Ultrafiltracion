**Actúa como un ingeniero en mecatrónica y programador experto en ESP32 y C++ (Entorno Arduino).**

Estoy desarrollando una tesis sobre ultrafiltración de agua y necesito escribir un código robusto y limpio para controlar una bomba peristáltica impulsada por un motor paso a paso.

**Hardware utilizado:**

* Microcontrolador: Nodemcu ESP32 de 38 pines.  
* Driver del motor: DM860 configurado a 1600 pulsos/rev y 3.15A.  
* Motor: Nema 34 (Serie Bipolar).  
* Conexión driver-ESP32: Configuración de "Cátodo Común" (PUL-, DIR- y ENA- punteados a GND del ESP32).

**Pines a utilizar en el ESP32:**

* `PUL+`(Pulso) conectado al **Pin 25** .  
* `DIR+`(Dirección) conectado al **Pin 26** .  
* `ENA+`(Habilitar) conectado al **Pin 27** .

**Requerimientos del Código:**

1. **Bluetooth Classic:** Utilizar la librería `BluetoothSerial.h`. El dispositivo debe llamarse "Bomba\_Filtro\_ESP32".  
2. **Control Serial:** El código debe escuchar los comandos enviados desde una aplicación de Terminal Bluetooth en un celular.  
3. **Lógica de comandos requeridos:**  
   * Enviando un `"1"`: Encender la bomba (activar enable y comenzar a generar pulsos).  
   * Enviando un `"0"`: Apagar la bomba (detener los pulsos y desactivar el enable por seguridad).  
   * Enviando una `"D"`o `"d"`: Cambiar el sentido de giro de la bomba (invertir el estado del pin DIR).  
   * Enviando un `"+"`: Aumentar la velocidad (reducir el retraso entre pulsos).  
   * Enviando un `"-"`: Disminuir la velocidad (aumentar el retraso entre pulsos).  
4. **Seguridad inicial:** Al arrancar el ESP32, la bomba debe estar estrictamente apagada.

Por favor, proporciona el código completo, comentado en español, explicando brevemente cómo funciona la generación de pulsos (con `digitalWrite`y `delayMicroseconds`) y cómo evitar que el bucle del motor bloquee la recepción de nuevos comandos Bluetooth.

/\*  
 \* \===================================================================  
 \* CONTROL DE BOMBA PERISTÁLTICA \- SISTEMA DE ULTRAFILTRACIÓN  
 \* \===================================================================  
 \* Microcontrolador: ESP32 (Nodemcu 38 pines)  
 \* Driver: DM860 (1600 pulsos/rev, 3.15A) \- Configuración Cátodo Común  
 \* Motor: NEMA 34 Bipolar  
 \* Control: Bluetooth Classic (SPP)  
 \* \===================================================================  
 \*/

\#include "BluetoothSerial.h"

// Verificación de que el Bluetooth Classic esté habilitado en la compilación  
\#if \!defined(CONFIG\_BT\_ENABLED) || \!defined(CONFIG\_BLUEDROID\_ENABLED)  
\#error Bluetooth is not enabled\! Por favor habilita "Bluetooth" en Tools \> Partition Scheme si aplica  
\#endif

BluetoothSerial SerialBT;

// \-------------------------------------------------------------------  
// DEFINICIÓN DE PINES  
// \-------------------------------------------------------------------  
const int PIN\_PUL \= 25;  // Pulso (PUL+) \-\> Genera los pasos del motor  
const int PIN\_DIR \= 26;  // Dirección (DIR+) \-\> Sentido de giro  
const int PIN\_ENA \= 27;  // Enable (ENA+) \-\> Habilita/deshabilita el driver

// \-------------------------------------------------------------------  
// VARIABLES DE ESTADO DEL MOTOR  
// \-------------------------------------------------------------------  
bool bombaEncendida \= false;   // Estado general: ON/OFF  
bool direccionActual \= LOW;    // Estado del pin DIR (false=LOW, true=HIGH)

// Variables para el control de velocidad (tiempo entre pulsos en microsegundos)  
unsigned int intervaloPulso \= 1000;   // Valor inicial (ajustable). Menor \= más rápido  
const unsigned int INTERVALO\_MIN \= 100;    // Límite superior de velocidad (evita perder pasos)  
const unsigned int INTERVALO\_MAX \= 10000;  // Límite inferior de velocidad (muy lento)  
const unsigned int PASO\_INCREMENTO \= 100;  // Cuánto cambia la velocidad con "+"/"-"

// \-------------------------------------------------------------------  
// VARIABLES PARA GENERACIÓN DE PULSOS NO BLOQUEANTE  
// \-------------------------------------------------------------------  
// En vez de usar delayMicroseconds() (que congela el procesador),  
// usamos micros() para llevar la cuenta del tiempo transcurrido.  
// Esto permite que el ESP32 siga "escuchando" el Bluetooth mientras  
// genera los pulsos del motor.  
unsigned long tiempoAnteriorPulso \= 0;  
bool estadoPinPulso \= LOW;

// El pulso necesita un tiempo mínimo en HIGH (ancho de pulso) antes de  
// volver a LOW. Para el DM860, unos pocos microsegundos son suficientes,  
// pero usamos un valor seguro.  
const unsigned int ANCHO\_PULSO\_US \= 5;

void setup() {  
  Serial.begin(115200);

  // Configuración de pines como salida  
  pinMode(PIN\_PUL, OUTPUT);  
  pinMode(PIN\_DIR, OUTPUT);  
  pinMode(PIN\_ENA, OUTPUT);

  // \-----------------------------------------------------------------  
  // SEGURIDAD INICIAL: La bomba SIEMPRE debe arrancar apagada.  
  // En cátodo común, generalmente ENA en LOW \= deshabilitado  
  // (esto depende del driver, pero DM860 en cátodo común:  
  //  ENA+ en HIGH habilita, ENA+ en LOW deshabilita)  
  // \-----------------------------------------------------------------  
  digitalWrite(PIN\_ENA, LOW);   // Motor deshabilitado (sin corriente al motor)  
  digitalWrite(PIN\_PUL, LOW);   // Sin pulsos  
  digitalWrite(PIN\_DIR, direccionActual); // Dirección por defecto

  bombaEncendida \= false;

  // Inicializa el Bluetooth con el nombre requerido  
  SerialBT.begin("Bomba\_Filtro\_ESP32");

  Serial.println("=================================================");  
  Serial.println("Sistema iniciado. Bomba APAGADA por seguridad.");  
  Serial.println("Bluetooth listo como: Bomba\_Filtro\_ESP32");  
  Serial.println("Comandos: 1=ON | 0=OFF | D=Dir | \+=Vel+ | \-=Vel-");  
  Serial.println("=================================================");  
}

void loop() {  
  // \-----------------------------------------------------------------  
  // 1\. ATENCIÓN A COMANDOS BLUETOOTH (siempre se revisa primero,  
  //    sin ningún delay que pueda retrasar la lectura)  
  // \-----------------------------------------------------------------  
  if (SerialBT.available()) {  
    char comando \= SerialBT.read();  
    procesarComando(comando);  
  }

  // \-----------------------------------------------------------------  
  // 2\. GENERACIÓN DE PULSOS NO BLOQUEANTE  
  //    Solo se ejecuta si la bomba está encendida.  
  // \-----------------------------------------------------------------  
  if (bombaEncendida) {  
    generarPulsoNoBloqueante();  
  }  
}

// \===================================================================  
// FUNCIÓN: procesarComando  
// Interpreta el carácter recibido por Bluetooth y ejecuta la acción  
// \===================================================================  
void procesarComando(char cmd) {  
  switch (cmd) {

    case '1':  // ENCENDER BOMBA  
      digitalWrite(PIN\_ENA, HIGH);  // Habilita el driver (envía corriente al motor)  
      bombaEncendida \= true;  
      tiempoAnteriorPulso \= micros(); // Reinicia el temporizador de pulsos  
      SerialBT.println("Bomba ENCENDIDA");  
      Serial.println("Bomba ENCENDIDA");  
      break;

    case '0':  // APAGAR BOMBA  
      bombaEncendida \= false;  
      digitalWrite(PIN\_PUL, LOW);   // Detiene cualquier pulso en curso  
      digitalWrite(PIN\_ENA, LOW);   // Deshabilita el driver (seguridad: sin corriente)  
      SerialBT.println("Bomba APAGADA");  
      Serial.println("Bomba APAGADA");  
      break;

    case 'D':  // CAMBIAR DIRECCIÓN  
    case 'd':  
      direccionActual \= \!direccionActual;      // Invierte el estado lógico  
      digitalWrite(PIN\_DIR, direccionActual);   // Aplica el cambio al pin DIR  
      SerialBT.println(direccionActual ? "Direccion: HORARIA" : "Direccion: ANTIHORARIA");  
      Serial.println(direccionActual ? "Direccion: HORARIA" : "Direccion: ANTIHORARIA");  
      break;

    case '+':  // AUMENTAR VELOCIDAD (reduce el intervalo entre pulsos)  
      if (intervaloPulso \> INTERVALO\_MIN) {  
        intervaloPulso \-= PASO\_INCREMENTO;  
        if (intervaloPulso \< INTERVALO\_MIN) intervaloPulso \= INTERVALO\_MIN;  
      }  
      SerialBT.print("Velocidad aumentada. Intervalo: ");  
      SerialBT.println(intervaloPulso);  
      Serial.print("Intervalo actual (us): ");  
      Serial.println(intervaloPulso);  
      break;

    case '-':  // DISMINUIR VELOCIDAD (aumenta el intervalo entre pulsos)  
      if (intervaloPulso \< INTERVALO\_MAX) {  
        intervaloPulso \+= PASO\_INCREMENTO;  
        if (intervaloPulso \> INTERVALO\_MAX) intervaloPulso \= INTERVALO\_MAX;  
      }  
      SerialBT.print("Velocidad disminuida. Intervalo: ");  
      SerialBT.println(intervaloPulso);  
      Serial.print("Intervalo actual (us): ");  
      Serial.println(intervaloPulso);  
      break;

    default:  
      // Ignora saltos de línea, retornos de carro u otros caracteres  
      // que algunas apps de terminal envían junto al comando.  
      break;  
  }  
}

// \===================================================================  
// FUNCIÓN: generarPulsoNoBloqueante  
// \-------------------------------------------------------------------  
// EXPLICACIÓN DEL FUNCIONAMIENTO:  
//  
// Un motor paso a paso avanza un "paso" cada vez que el driver detecta  
// un flanco de subida (LOW \-\> HIGH) en el pin PUL. La forma tradicional  
// de generar esto es:  
//  
//      digitalWrite(PIN\_PUL, HIGH);  
//      delayMicroseconds(x);  
//      digitalWrite(PIN\_PUL, LOW);  
//      delayMicroseconds(x);  
//  
// El problema es que delayMicroseconds() BLOQUEA por completo el  
// procesador: mientras se ejecuta, el ESP32 no puede revisar si llegó  
// un comando Bluetooth nuevo. Si la bomba debe girar rápido y por  
// mucho tiempo, el "1" nunca se detendría hasta que decidas cortar el  
// código, ¡y el "0" jamás sería leído a tiempo\!  
//  
// SOLUCIÓN: Usamos micros() (el reloj interno de microsegundos del  
// ESP32) para comparar cuánto tiempo ha pasado desde el último cambio  
// de estado del pin. Si ya pasó el tiempo necesario, cambiamos el pin  
// de estado (esto se llama "máquina de estados"). Como cada llamada a  
// esta función dura microsegundos y luego regresamos al loop(), el  
// ESP32 tiene tiempo de sobra para revisar el Bluetooth en cada vuelta.  
// \===================================================================  
void generarPulsoNoBloqueante() {  
  unsigned long ahora \= micros();

  if (estadoPinPulso \== LOW) {  
    // Si el pin está en LOW, verificamos si ya pasó el intervalo  
    // completo para iniciar un nuevo pulso (flanco de subida)  
    if (ahora \- tiempoAnteriorPulso \>= intervaloPulso) {  
      digitalWrite(PIN\_PUL, HIGH);  
      estadoPinPulso \= HIGH;  
      tiempoAnteriorPulso \= ahora;  
    }  
  } else {  
    // Si el pin está en HIGH, solo necesitamos mantenerlo así por  
    // un tiempo muy corto (ANCHO\_PULSO\_US) antes de bajarlo a LOW.  
    if (ahora \- tiempoAnteriorPulso \>= ANCHO\_PULSO\_US) {  
      digitalWrite(PIN\_PUL, LOW);  
      estadoPinPulso \= LOW;  
      tiempoAnteriorPulso \= ahora;  
    }  
  }  
}  
