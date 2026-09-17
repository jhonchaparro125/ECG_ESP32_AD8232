#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// =====================================================
//                    TFT ST7789V3
// =====================================================

#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_SCLK  18
#define TFT_MOSI  23

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);


// =====================================================
//                       AD8232
// =====================================================

#define ECG_PIN   34
#define LO_PLUS   32
#define LO_MINUS  33


// =====================================================
//                       COLORES
// =====================================================

#define VERDE   0x07E0
#define BLANCO  0xFFFF
#define NEGRO   0x0000
#define GRIS    0x4208


// =====================================================
//                    PANTALLA
// =====================================================

#define ANCHO 280
#define ALTO  240

#define Y_SUPERIOR 75
#define Y_INFERIOR 210
#define Y_CENTRO   145


// =====================================================
//                VELOCIDAD DE LA GRÁFICA
// =====================================================

#define MUESTRAS_PIXEL 2

int contadorMuestras = 0;

int xActual = 0;
int yAnterior = Y_CENTRO;


// =====================================================
//                  FILTRO DE SEÑAL
// =====================================================

float lineaBase = 2048.0;

float senalFiltrada = 0;

float filtroAnterior = 0;


// =====================================================
//              FILTRO NOTCH 60 Hz
// =====================================================

float notch_x1 = 0;
float notch_x2 = 0;
float notch_y1 = 0;
float notch_y2 = 0;


// =====================================================
//                  GANANCIA GRÁFICA
// =====================================================

float ganancia = 0.20;


// =====================================================
//                     MUESTREO
// =====================================================

#define PERIODO_MUESTREO 4000

unsigned long tiempoMuestreo = 0;


// =====================================================
//                        BPM
// =====================================================

int BPM = 0;

unsigned long ultimoPico = 0;

bool picoDetectado = false;


// =====================================================
//               DETECTOR DE PICOS
// =====================================================

float nivelSenal = 20;

float umbralPico = 100;


// =====================================================
//            INTERVALO ENTRE LATIDOS
// =====================================================

// 500 ms evita que una onda T cercana
// sea tomada como otro latido.

#define INTERVALO_MIN 500

// 2000 ms = 30 BPM

#define INTERVALO_MAX 2000


// =====================================================
//               PROMEDIO DE LATIDOS
// =====================================================

#define NUM_INTERVALOS 5

unsigned long intervalos[NUM_INTERVALOS];

int cantidadIntervalos = 0;


// =====================================================
//                    PROTOTIPOS
// =====================================================

void detectarLatido(float senal);

void guardarIntervalo(unsigned long intervalo);

void mostrarBPM();

void dibujarECG(int y);

void dibujarCabecera();

float filtroNotch60Hz(float entrada);


// =====================================================
//                       SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  delay(500);


  // ---------------------------------------------------
  // AD8232
  // ---------------------------------------------------

  pinMode(LO_PLUS, INPUT);
  pinMode(LO_MINUS, INPUT);

  analogReadResolution(12);


  // ---------------------------------------------------
  // SPI
  // ---------------------------------------------------

  SPI.begin(
    TFT_SCLK,
    -1,
    TFT_MOSI,
    TFT_CS
  );


  // ---------------------------------------------------
  // TFT
  // ---------------------------------------------------

  tft.init(240, 280);

  tft.setRotation(1);

  tft.fillScreen(NEGRO);


  // ---------------------------------------------------
  // CABECERA
  // ---------------------------------------------------

  dibujarCabecera();


  // ---------------------------------------------------
  // LÍNEA CENTRAL
  // ---------------------------------------------------

  tft.drawFastHLine(
    0,
    Y_CENTRO,
    ANCHO,
    GRIS
  );


  // ---------------------------------------------------
  // TIEMPO
  // ---------------------------------------------------

  tiempoMuestreo = micros();


  Serial.println();
  Serial.println("================================");
  Serial.println("          MONITOR ECG");
  Serial.println("       ESP32 + AD8232");
  Serial.println("================================");
}


// =====================================================
//                 FILTRO NOTCH 60 Hz
// =====================================================

float filtroNotch60Hz(float entrada)
{
  float fs = 250.0;

  float f0 = 60.0;

  float Q = 8.0;

  float w0 =
    2.0 * PI * f0 / fs;

  float alpha =
    sin(w0) / (2.0 * Q);


  // Coeficientes

  float b0 = 1.0;
  float b1 = -2.0 * cos(w0);
  float b2 = 1.0;

  float a0 = 1.0 + alpha;
  float a1 = -2.0 * cos(w0);
  float a2 = 1.0 - alpha;


  // Normalizar

  b0 /= a0;
  b1 /= a0;
  b2 /= a0;

  a1 /= a0;
  a2 /= a0;


  // Ecuación del filtro

  float salida =
      b0 * entrada
    + b1 * notch_x1
    + b2 * notch_x2
    - a1 * notch_y1
    - a2 * notch_y2;


  // Actualizar memoria

  notch_x2 = notch_x1;
  notch_x1 = entrada;

  notch_y2 = notch_y1;
  notch_y1 = salida;


  return salida;
}


// =====================================================
//                       LOOP
// =====================================================

void loop()
{
  unsigned long ahora = micros();


  // ===================================================
  // MUESTREO CONSTANTE
  // ===================================================

  if (
    ahora - tiempoMuestreo >=
    PERIODO_MUESTREO
  )
  {
    tiempoMuestreo +=
      PERIODO_MUESTREO;


    // =================================================
    // LEER AD8232
    // =================================================

    int lecturaRaw =
      analogRead(ECG_PIN);


    // =================================================
    // DETECTAR ELECTRODOS
    // =================================================

    bool electrodosOK =
      (
        digitalRead(LO_PLUS) == LOW &&
        digitalRead(LO_MINUS) == LOW
      );


    // =================================================
    // FILTRAR LÍNEA BASE
    // =================================================

    lineaBase =
      lineaBase +
      0.005 *
      (
        lecturaRaw -
        lineaBase
      );


    float senalSinBase =
      lecturaRaw -
      lineaBase;


    // =================================================
    // FILTRO NOTCH 60 Hz
    // =================================================

    float señalNotch =
      filtroNotch60Hz(
        senalSinBase
      );


    // =================================================
    // FILTRO SUAVIZADO
    // =================================================

    senalFiltrada =
      filtroAnterior * 0.85 +
      señalNotch * 0.15;


    filtroAnterior =
      senalFiltrada;


    // =================================================
    // SERIAL
    // =================================================

    Serial.println(
      senalFiltrada
    );


    // =================================================
    // CALCULAR BPM
    // =================================================

    if (electrodosOK)
    {
      detectarLatido(
        senalFiltrada
      );
    }


    // =================================================
    // GRÁFICA
    // =================================================

    contadorMuestras++;


    if (
      contadorMuestras >=
      MUESTRAS_PIXEL
    )
    {
      contadorMuestras = 0;


      // -----------------------------------------------
      // Convertir señal a coordenada Y
      // -----------------------------------------------

      int y =
        Y_CENTRO -
        (
          senalFiltrada *
          ganancia
        );


      // -----------------------------------------------
      // Limitar pantalla
      // -----------------------------------------------

      y =
        constrain(
          y,
          Y_SUPERIOR,
          Y_INFERIOR
        );


      // -----------------------------------------------
      // Dibujar
      // -----------------------------------------------

      dibujarECG(y);
    }
  }
}


// =====================================================
//                  CABECERA TFT
// =====================================================

void dibujarCabecera()
{
  tft.fillRect(
    0,
    0,
    ANCHO,
    45,
    NEGRO
  );


  // ---------------------------------------------------
  // Título
  // ---------------------------------------------------

  tft.setTextColor(
    VERDE,
    NEGRO
  );

  tft.setTextSize(2);

  tft.setCursor(
    8,
    12
  );

  tft.print("ECG");


  // ---------------------------------------------------
  // Texto BPM
  // ---------------------------------------------------

  tft.setTextSize(1);

  tft.setCursor(
    205,
    5
  );

  tft.print("BPM");


  mostrarBPM();
}


// =====================================================
//                    MOSTRAR BPM
// =====================================================

void mostrarBPM()
{
  // Borrar número anterior

  tft.fillRect(
    205,
    15,
    70,
    28,
    NEGRO
  );


  tft.setTextColor(
    BLANCO,
    NEGRO
  );

  tft.setTextSize(3);


  if (BPM <= 0)
  {
    tft.setCursor(
      225,
      16
    );

    tft.print("--");
  }
  else
  {
    if (BPM < 10)
    {
      tft.setCursor(
        250,
        16
      );
    }
    else if (BPM < 100)
    {
      tft.setCursor(
        230,
        16
      );
    }
    else
    {
      tft.setCursor(
        210,
        16
      );
    }

    tft.print(BPM);
  }
}


// =====================================================
//                    DIBUJAR ECG
// =====================================================

void dibujarECG(int y)
{
  // ---------------------------------------------------
  // Llegar al final de la pantalla
  // ---------------------------------------------------

  if (
    xActual >= ANCHO
  )
  {
    xActual = 0;

    yAnterior =
      Y_CENTRO;
  }


  // ---------------------------------------------------
  // Borrar solamente la columna actual
  // ---------------------------------------------------

  tft.drawFastVLine(
    xActual,
    Y_SUPERIOR,
    Y_INFERIOR -
    Y_SUPERIOR,
    NEGRO
  );


  // ---------------------------------------------------
  // Línea central
  // ---------------------------------------------------

  tft.drawPixel(
    xActual,
    Y_CENTRO,
    GRIS
  );


  // ---------------------------------------------------
  // Dibujar señal
  // ---------------------------------------------------

  if (
    xActual > 0
  )
  {
    tft.drawLine(
      xActual - 1,
      yAnterior,
      xActual,
      y,
      VERDE
    );
  }


  yAnterior =
    y;

  xActual++;
}


// =====================================================
//                 DETECTAR LATIDO
// =====================================================

void detectarLatido(float senal)
{
  unsigned long ahora =
    millis();


  // ---------------------------------------------------
  // Parte positiva de la señal
  // ---------------------------------------------------

  float senalPositiva =
    senal;

  if (
    senalPositiva < 0
  )
  {
    senalPositiva = 0;
  }


  // ---------------------------------------------------
  // Nivel promedio
  // ---------------------------------------------------

  nivelSenal =
    nivelSenal * 0.995 +
    senalPositiva * 0.005;


  // ---------------------------------------------------
  // Umbral dinámico
  // ---------------------------------------------------

  umbralPico =
    nivelSenal * 2.8;


  if (
    umbralPico < 80
  )
  {
    umbralPico = 80;
  }


  if (
    umbralPico > 450
  )
  {
    umbralPico = 450;
  }


  // ===================================================
  // DETECCIÓN DEL PICO
  // ===================================================

  if (
    senalPositiva > umbralPico &&
    !picoDetectado
  )
  {
    // -----------------------------------------------
    // Evitar picos demasiado cercanos
    // -----------------------------------------------

    if (
      ultimoPico == 0 ||
      ahora - ultimoPico >=
      INTERVALO_MIN
    )
    {
      picoDetectado =
        true;


      // ---------------------------------------------
      // Primer pico
      // ---------------------------------------------

      if (
        ultimoPico == 0
      )
      {
        ultimoPico =
          ahora;

        return;
      }


      // ---------------------------------------------
      // Intervalo
      // ---------------------------------------------

      unsigned long intervalo =
        ahora -
        ultimoPico;


      // ---------------------------------------------
      // VALIDAR INTERVALO
      // ---------------------------------------------

      if (
        intervalo >=
        INTERVALO_MIN &&
        intervalo <=
        INTERVALO_MAX
      )
      {
        guardarIntervalo(
          intervalo
        );
      }


      // ---------------------------------------------
      // IMPORTANTE:
      // actualizar siempre el último pico aceptado
      // ---------------------------------------------

      ultimoPico =
        ahora;
    }
  }


  // ===================================================
  // REARMAR DETECTOR
  // ===================================================

  if (
    senalPositiva <
    umbralPico * 0.40
  )
  {
    picoDetectado =
      false;
  }
}


// =====================================================
//              GUARDAR INTERVALO
// =====================================================

void guardarIntervalo(
  unsigned long intervalo
)
{
  // ---------------------------------------------------
  // Mover historial
  // ---------------------------------------------------

  for (
    int i = NUM_INTERVALOS - 1;
    i > 0;
    i--
  )
  {
    intervalos[i] =
      intervalos[i - 1];
  }


  // ---------------------------------------------------
  // Guardar nuevo intervalo
  // ---------------------------------------------------

  intervalos[0] =
    intervalo;


  // ---------------------------------------------------
  // Cantidad de intervalos
  // ---------------------------------------------------

  if (
    cantidadIntervalos <
    NUM_INTERVALOS
  )
  {
    cantidadIntervalos++;
  }


  // ---------------------------------------------------
  // Calcular promedio
  // ---------------------------------------------------

  unsigned long suma =
    0;


  for (
    int i = 0;
    i < cantidadIntervalos;
    i++
  )
  {
    suma +=
      intervalos[i];
  }


  unsigned long promedio =
    suma /
    cantidadIntervalos;


  // ---------------------------------------------------
  // Calcular BPM
  // ---------------------------------------------------

  int nuevoBPM =
    60000 /
    promedio;


  // ---------------------------------------------------
  // Validar BPM
  // ---------------------------------------------------

  if (
    nuevoBPM >= 40 &&
    nuevoBPM <= 180
  )
  {
    BPM =
      nuevoBPM;


    mostrarBPM();


    Serial.print(
      "BPM: "
    );

    Serial.println(
      BPM
    );
  }
}