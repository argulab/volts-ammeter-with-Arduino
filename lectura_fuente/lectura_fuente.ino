#include <Wire.h>                                                                                                                             #include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define PIN_VOLTS A0
#define PIN_AMPS A1

unsigned long check_init = -1;
float volts = 0;
float amps = -1;
float max_amps = 0;
float min_volts = 16;

LiquidCrystal_I2C lcd(0x27, 20, 4);

byte char_gauge[] = {
  B1111,
  B1111,
  B1111,
  B1111,
  B1111,
  B1111,
  B1111,
  B1111
};

float read_analog_data(byte pin) {
  float steps = 100.0;
  float raw_total = 0.0;

  for (int i = 0; i <= 100; i++) {
    raw_total += analogRead(pin);
  }  
  return raw_total / steps;
}

float raw_to_volts(float raw) {
  float max_raw = 1023.0;
  float max_volts = 5.0;
  return (raw * max_volts) / max_raw;
}

float volts_to_amps(float v) {
  float negative_volts = 2.5;
  float volts_per_amp = 0.066;
  
  return (v - negative_volts) / volts_per_amp;
}

float volts_divider() {
  float resistor_one = 2708.0;
  float resistor_two = 1192.0;
  return resistor_two / (resistor_one + resistor_two);
}

void display_volts(float v) {
  lcd.setCursor(2, 0);
  lcd.print("     ");
  lcd.setCursor(2, 0);
  lcd.print(v, 2);   
}

void display_amps(float a) {
  lcd.setCursor(11, 0);
  lcd.print("     ");
  lcd.setCursor(11, 0);
  lcd.print(a, 2);  
}

void display_gauge(float a) {
  int max_gauge = 25;
  int total_gauge = int((a * max_gauge) / 100);

  for (int c = 0; c <= 9; c++) {
    lcd.setCursor(c, 1);
    if (c <= total_gauge) {
      lcd.write(byte(0));
    } else {
      lcd.print(" ");
    }
  }
}

void display_max_amps(float a) {
  lcd.setCursor(11, 1);
  lcd.print("     ");
  lcd.setCursor(11, 1);
  lcd.print(a, 2); 
}

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

void service_volts() {
  float split_volts = 0.1;
  float v_divider = volts_divider();
  float r_volts = read_analog_data(PIN_VOLTS);
  float n_volts = raw_to_volts(r_volts);
  float new_volts = n_volts / v_divider;

  if((new_volts > (volts + split_volts)) || (new_volts < (volts - split_volts))) {
    display_volts(new_volts);
    volts = new_volts;
    delay(500);
  }

  if (volts < min_volts) {
    Serial.print("Tensión mínima = ");
    Serial.println(volts);
    min_volts = volts;
  }
}

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
  int lapso = 5;
  if ((millis() - check_init) > (lapso * 1000)) {
    max_amps = amps;
    check_init = millis();
    display_gauge(amps);
    display_max_amps(amps);
  }
  service_volts();
  service_amps();
}
