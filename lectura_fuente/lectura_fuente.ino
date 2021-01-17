#include <Wire.h>                                                                                                                             #include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define PIN_VOLTS A0
#define PIN_AMPS A1

/*
 * Variables globales
 */
unsigned long check_init = -1;  //Inicio del período de vigencia del pico máximo de consumo
float volts = 0;                //Última lectura de tensión
float amps = -1;                //Última lectura de consumo
float max_amps = 0;             //Pico de consumo
float min_volts = 16;           //Mínima tensión 

LiquidCrystal_I2C lcd(0x27, 20, 4);

byte char_gauge[] = { //Crea un caracter especial para la barra deslizante byte(0)
  B1111,
  B1111,
  B1111,
  B1111,
  B1111,
  B1111,
  B1111,
  B1111
};

/*
 * Realiza 100 lecturas de I/O pin y devuelve el promedio
 */
float read_analog_data(byte pin) {
  float steps = 100.0;
  float raw_total = 0.0;

  for (int i = 0; i <= 100; i++) {
    raw_total += analogRead(pin);
  }  
  return raw_total / steps;
}

/*
 * Recibe la lectura en segmentos digitales y devuelve la tensión
 */
float raw_to_volts(float r) {
  float max_raw = 1023.0;
  float max_volts = 5.0;
  return (r * max_volts) / max_raw;
}

/*
 * Recibe la tensión del ACS-714 y devuelve el consumo en amperios
 */
float volts_to_amps(float v) {
  float negative_volts = 2.5;
  float volts_per_amp = 0.066;
  
  return (v - negative_volts) / volts_per_amp;
}

/*
 * Calcula el factor de division en la entrada de tensión en A0
 */
float volts_divider() {
  float resistor_one = 2708.0;
  float resistor_two = 1192.0;
  return resistor_two / (resistor_one + resistor_two);
}

/*
 * Presenta la tensión con dos decimales en la posición 2 de la línea 0 del display
 */
void display_volts(float v) {
  lcd.setCursor(2, 0);
  lcd.print("     ");
  lcd.setCursor(2, 0);
  lcd.print(v, 2);   
}

/*
 * Presenta el consumo actual en la posición 11 de la línea 0
 */
void display_amps(float a) {
  lcd.setCursor(11, 0);
  lcd.print("     ");
  lcd.setCursor(11, 0);
  lcd.print(a, 2);  
}

/*
 * Presenta una barra dinámica del consumo actual con una relación de 25A por 10 posiciones (2,5A por dígito
 * a partir de la posición 0 a la 9 de la línea 1
 */
void display_gauge(float a) {
  int max_amps_gauge = 25;
  int max_col_gauge = 10;
  int total_gauge_write_in = int((a * max_col_gauge) / max_amps_gauge);

  for (int col = 1; col <= max_col_gauge; col++) {
    lcd.setCursor(col - 1, 1);
    if (col <= total_gauge_write_in) {
      lcd.write(byte(0));
    } else {
      lcd.print(" ");
    }
  }
}

/*
 * Presenta el consumo máximo instantáneo
 */
void display_max_amps(float a) {
  lcd.setCursor(11, 1);
  lcd.print("     ");
  lcd.setCursor(11, 1);
  lcd.print(a, 2); 
}

/*
 * Presenta un mensaje inicial
 */
void init_message() {
  lcd.setCursor(0, 0);
  lcd.print("MEDIDOR VOLT/AMP");
  lcd.setCursor(0, 1);
  lcd.print("EA2J ** ENE-2021");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V=");
  lcd.setCursor(9, 0);
  lcd.print("A=");
}

/*
 * Ejecuta las instrucciones oara el proceso de lectura de la tensión
 */
void service_volts() {
  float split_volts = 0.1;                      //Define la desviación máxima para un refresco de pantalla
  float v_divider = volts_divider();            //Define el factor de división de la tensión de entrada
  float r_volts = read_analog_data(PIN_VOLTS);  //Fuerza la lectura de la entrada analógica PIN_VOLTS
  float n_volts = raw_to_volts(r_volts);        //Transforma la lectura de la puerta en voltios
  float new_volts = n_volts / v_divider;        //Calcula los voltios de entrada antes del divisor resistivo

  if((new_volts > (volts + split_volts)) || (new_volts < (volts - split_volts))) { 
    display_volts(new_volts);                   //Presenta la nueva lectura
    volts = new_volts;                          //Refresca el valor de la variable global amps
    delay(500);                                
  }

  if (volts < min_volts) {                //Presenta la tensión mínima registrada en el monitor serie
    Serial.print("Tensión mínima = ");
    Serial.println(volts);
    min_volts = volts;
  }
}

/*
 * Ejecuta las instrucciones para la lectura y presentación del consumo
 */
void service_amps() {
  float split_amps = 0.1;
  float r_amps = read_analog_data(PIN_AMPS);
  float v_amps = raw_to_volts(r_amps);
  float new_amps = volts_to_amps(v_amps);

  if ((new_amps > (amps + split_amps)) || (new_amps < (amps - split_amps))) {
    display_amps(new_amps);
    display_gauge(new_amps);
    amps = new_amps;
    delay(200); 
  }
  if (amps > max_amps) {
    display_max_amps(amps);
    max_amps = amps;
    check_init = millis();
  }
}

void setup() {
  Serial.begin(9600);
  Serial.flush();
  check_init = millis();
  lcd.createChar(0, char_gauge);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  init_message();
}

void loop() {
  int lapso = 5; //Establece el lapso de tiempo en segundos sin que se haya registrado un cambio de valor
  if ((millis() - check_init) > (lapso * 1000)) { //Si han pasaso más de 5 segundos
    max_amps = amps;                              //Refresca el valor de max_amps (global)
    check_init = millis();                        //Refresca el valor de check_init (global)
    display_gauge(max_amps);
    display_max_amps(max_amps);
  }
  service_volts();
  service_amps();
}
