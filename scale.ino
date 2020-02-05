#include <HX711.h>
#include <SoftwareSerial.h>



/*Ağırlık Sensörü Değişkenleri*/
float calibration_factor = 22900;     // Kendi Ağırlık sensörünüze göre ayarlamalısınız
float olcum;                          // Ölçülen Ağırlık, Sensörden gelir.
double enb;                           //En büyük ölçülen ağırlık değeri
int checkcount=0;                     // En büyük ölçülen ağırlıktan az ölçülen ölçümleri sayar
boolean isTrapActive=false;           // Bluetooth ile değiştirilir alarm modu devredışıyken false olur.
/*Wifi ESP8266 Modülü Değişkenleri*/
int rxPin=4;
int txPin=5;
String agAdi = "ErhanWifi";                 //Ağ adı    
String agSifresi = "160201039Erhan";        //Ağ şifresi
/*HC05 Modülü Değişkenleri*/
int enablePin=8;
/*ThingSpeak Değişkenleri*/
String TsIp="184.106.153.149";         //Thingspeak ip adresi.
String tsHttpApiKey="****************";
String oku;
String appMode="Alarm";               //Alarm modunu belirtir.
/*Hareket Sensörü değişkenleri*/
int hareketPin = 12;               // Hareket Sensörü data pini
int hareket = LOW;             //     Başlangıçta hareket yok
bool hareketOlc=false;


HX711 agirlik(3, 2);                    //HX711 i başlatır (Ağırlık Sensörleri).
SoftwareSerial esp(rxPin, txPin);     //esp için seri port
SoftwareSerial hc05(10,9);            //bluetooth için seri port


void setup() {  
  Serial.begin(9600);
  hx711_kur();
  esp8266_kur();
  pinMode(hareketPin, INPUT);
  hc05.begin(19200);
  hc05.listen();
  
}

void loop() {
  agirlik_olc();
  check();
  if(hareketOlc == true && appMode=="Alarm" && isTrapActive)
    hareket_kontrol();
    if(hareket==HIGH){
      esp.listen();
      gonder(1);
      hareket=LOW;
      delay(1000);
    }
  delay(100);
  if(enb>5 && checkcount>20 && isTrapActive && appMode=="Alarm"){
    esp.listen();
    gonder(2);
  }
  if(appMode=="Tarti"){
    hc05.print(olcum);
    delay(1000);
  }
  oku="";
  bt_kontrol();
}

void gonder(int sensor){
   
      Serial.println(String(enb)+"  "+String(checkcount));
      again_wifi: 
      esp.println("AT+CIPSTART=\"TCP\",\""+TsIp+"\",80");
      if(esp.find("Error")){                                      //Bağlantı hatası kontrolü yapıyoruz.
          Serial.println("AT+CIPSTART Error");
      }
      String veri = "GET https://api.thingspeak.com/update?api_key=***************";
      if(sensor==2){
        veri += "&field2=";
        veri += String(enb);                                       
        veri += "\r\n\r\n";
      }
      else if(sensor==1){
        veri += "&field1=1";
        veri += "\r\n\r\n";
      }
      esp.print("AT+CIPSEND=");                                   //ESP'ye göndereceğimiz veri uzunluğunu veriyoruz.
      esp.println(veri.length()+2);
      delay(500);
      if(esp.find(">")){                                         
          esp.print(veri);                                        //Veriyi gönderiyoruz.
          Serial.println(veri);
          Serial.println("Veri gonderildi.");
          delay(100);
          Serial.println("Baglantı Kapatildi.");
          esp.println("AT+CIPCLOSE"); 
          delay(100);
      }
      else{
          Serial.print(".");
          esp.flush();
          goto again_wifi;
      }
      enb=0;
      checkcount=0;
      hc05.listen();

    
}
void esp8266_kur(){
  int timer=0;
  again:                                   //ESP8266 ile seri haberleşmeyi başlatıyoruz.
  esp.begin(115200);    
  delay(100);                          
  Serial.println("AT Yollandı");                              //AT komutu ile modül kontrolünü yapıyoruz.
  while(!esp.find("OK")){                                     //Modül hazır olana kadar bekliyoruz.
    if(timer>10){
      timer=0;
      Serial.println("ESP8266 Tekrar Bulunmaya Çalışılıyor");
      goto again;
    }
    esp.println("AT");
    Serial.println("ESP8266 Bulunamadı.");
    timer++;
  }
  timer=0;
  Serial.println("OK Komutu Alındı");
  esp.println("AT+CWMODE=1");                                 //ESP8266 modülünü client olarak ayarlıyoruz.
  while(!esp.find("OK")){                                     //Ayar yapılana kadar bekliyoruz.
    if(timer>10){
      timer=0;
      Serial.println("Client olarak ayarlama tekrar deneniyor.");
      goto again;
    }
    esp.println("AT+CWMODE=1");
    Serial.println("Ayar Yapılıyor....");
    timer++;
  }
  timer=0;
  Serial.println("Client olarak ayarlandı");
  connect_again:
  Serial.println("Aga Baglaniliyor...");
  Serial.println("AT+CWJAP=\""+agAdi+"\",\""+agSifresi+"\"");
  esp.println("AT+CWJAP=\""+agAdi+"\",\""+agSifresi+"\"");    //Ağımıza bağlanıyoruz.
  while(!esp.find("OK")){
    if(timer>20){
      timer=0;
      Serial.println("Aga Baglanma Tekrar Deneniyor");
      goto connect_again;
    }
    Serial.print(".");
    timer++;
    }                                     //Ağa bağlanana kadar bekliyoruz.
    timer=0;
  Serial.println("Aga Baglandi.");
  delay(1000);
}


/*Ağırlık Sensörünün Kurulumu*/
void hx711_kur(){
  Serial.println("HX711 Başlatılıyor.");
  agirlik.set_scale();
  agirlik.tare();  //Dara alma işlemi
  long zero_factor = agirlik.read_average();
  Serial.print("Zero factor: "); 
  Serial.println(zero_factor);
}

 void agirlik_olc(){
  agirlik.set_scale(calibration_factor); //Kalibrasyon faktörünün atanması
  Serial.print("Agirlik: ");
  olcum = agirlik.get_units(), 10;      //Ağırlık değeri okunuyor
  if (olcum < 0)
  {
    olcum = 0.00;
  }
  Serial.print(olcum);
  Serial.print(" kg"); 
  Serial.println();
}


void bt_kontrol(){
  if(hc05.available()){
    while(hc05.available()){
    oku+=char(hc05.read());  
  }
  Serial.println(oku);
  if(oku=="Etkin"){
    isTrapActive=true;
    oku="";
    enb=0;
    checkcount=0;
  }
  else if(oku=="Dur"){
    isTrapActive=false;
    oku="";
    enb=0;
    checkcount=0;
  }
  else if(oku=="Alarm"){
    isTrapActive=false;
    appMode="Alarm";
    oku="";
    enb=0;
    checkcount=0;
    
    
  }
  else if(oku=="Tarti"){
    isTrapActive=false;
    appMode="Tarti";
    oku="";
    enb=0;
    checkcount=0; 
  }
  else if(oku=="hareketEtkin"){
    hareketOlc=true;
    oku="";
    enb=0;
    checkcount=0; 
  }
  else if(oku=="hareketDur"){
    hareketOlc=false;
    oku="";
    enb=0;
    checkcount=0; 
  }
  hc05.flush();
  }
  
}
void hareket_kontrol(){
 int okunan = digitalRead(hareketPin);
  if (okunan == HIGH && hareket == LOW)
  {            
      Serial.println("Hareket Başladı");
      hareket = HIGH;
  } 
  else if(okunan == LOW && hareket == HIGH)
  {
      Serial.println("Hareket Bitti");
      hareket = LOW;
  }
}

// check fonksiyonu agirligin thresholdu geçtiği an buluta gonderilmesini engeller en buyuk agirligi tutar.
void check() {
  if(enb<olcum){
    enb=olcum;
    checkcount=0;
  }
  else if(enb>olcum){
    checkcount+=1;
  }
}

void hc05_at_mode(){
    if (hc05.available())
    Serial.write(hc05.read());
  if (Serial.available()){
    hc05.write(Serial.read());
  }
}
  
