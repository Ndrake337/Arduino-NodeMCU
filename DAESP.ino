#include <Ultrasonic.h>
#include <Wire.h>
#include <SoftwareSerial.h>

//Autores: Eric Kei, Matheus Felipe, Rafael Prado
//Turma: Engenharia da Computação - 2º Período

//Pinos
#define trigger1 9  //Pino do gatilho do ultrassom                      
#define echo1 8     //Pino do sensor de eco do ultrassom
#define trigger2 7
#define echo2 6
#define DEBUG true

const int pinoSensor = A0;          //Pino do sensor de umidade
const int porta_rele1 = 10;         //Tanque da chuva (pinoValvula) 
const int porta_rele2 = 13;         //Tanque comum (pinoValvula2)
Ultrasonic ultrassom(trigger1,echo1);
Ultrasonic ultrassom2(trigger2,echo2);
SoftwareSerial esp8266(5, 4);

//Variáveis modificáveis
const int limiarSeco = 30; //Valor máximo para ser considerado seco
const int tempoRega = 4;   //Tempo de regagem EM SEGUNDOS
const int h = 11;          //DETERMINAR A ALTURA DO RESERVATÓRIO EM CM!!!!!!!!!!!!!!!!!
const int a = 128.25;           //DETERMINAR A ÁREA DA BASE DO RESERVATÓRIO EM CM2!!!!!!!!!!

//Variáveis de recurso -- NÃO MODIFICAR
float distancia = 0;                    
float tempo = 0;
float distancia2 = 0;                    
float tempo2 = 0;
int umidadeSolo = 0;
long microsec = 0;
float cmMsec = 0;

String sendData(String command, const int timeout, boolean debug)
{
  // Envio dos comandos AT para o modulo
  String response = "";
  esp8266.print(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (esp8266.available())
    {
      // The esp has data so display its output to the serial window
      char c = esp8266.read(); // read the next character.
      response += c;
    }
  }
  if (debug)
  {
    Serial.print(response);
  }
  return response;
}

void wifi(float A, float B, float C,int D)
{
  if (esp8266.available() && esp8266.find("+IPD,"))
  {
    //if (esp8266.find("+IPD,"))
    
      delay(1500);
      int connectionId = esp8266.read() - 48;
 
      String webpage = "<head><meta http-equiv=""refresh"" content=""3""><title>Eco-Irrigação - DAESP </title>";
      webpage += "</head><h1><u>Eco-Irrigacao - DAESP</u></h1>";
      webpage += "<h2>Nivel de agua reservatorio (agua normal) ";
      float a = A; //distancia;//digitalRead(0); //le o retorno do sensor de ultrassionico posicionado no reservatório contendo água normal
      webpage += (String)a; //printa o dado obitido no reservatório de água normal 
      webpage += " L";
      webpage += "</h2>";

      webpage += "<h2>Nivel da agua do reservatorio (agua da chuva): ";
      float b = B;//distancia2;//digitalRead(8); //le o retorno do sensor de ultrassonico posicionado no reservatório contendo água da chuva
      webpage += (String)(b); //printa o dado obitido no reservatório de água da chuva
      webpage += " L";
      webpage += "</h2>";
      
      
      webpage += "<h2>Umidade do solo: "; 
      float c = C;//umidadeSolo;//analogRead(0); //le o retorno do sensor de umidade do solo
      webpage += (String)c; //printa o dado obitido no sensor de umidade do solo
      webpage += " %";
      webpage += "</h2>";

      if (D == 1)
      {
        webpage += "</head><h1>    Regando</h1>";
      }
      else if (D == 2)
      {
        webpage += "</head><h1>    Solo Encharcado</h1>";
      }
 
      String cipSend = "AT+CIPSEND=";
      cipSend += connectionId;
      cipSend += ",";
      cipSend += webpage.length();
      cipSend += "\r\n";
 
      sendData(cipSend, 1000, DEBUG);
      sendData(webpage, 1000, DEBUG);
 
      String closeCommand = "AT+CIPCLOSE=";
      closeCommand += connectionId; // append connection id
      closeCommand += "\r\n";
 
      sendData(closeCommand, 3000, DEBUG);
    
  }
}

//FUNCAO PARA VER O VOLUME (i: reservatório; h: altura;a: area da base)
float volume(int i, float h, float a)
{
  float v = 0, d = 0, sla; //v: volume;d: distância com água
  float m[5];
  if (i == 1)
  {
    for(int j = 0; j < 5; j++)
    {
     digitalWrite(trigger1, HIGH);
     delayMicroseconds(10);
     digitalWrite(trigger1, LOW);
     m[j] = h - ultrassom.Ranging(CM);
     if (m[j] < 0)
      m[j] = 0;
     delay(400);
    }
  }
  else if (i == 2)
  {
    for(int j = 0; j < 5; j++)
    {
     digitalWrite(trigger2, HIGH);
     delayMicroseconds(10);
     digitalWrite(trigger2, LOW);
     m[j] = h - ultrassom2.Ranging(CM);
     if (m[j] < 0)
      m[j] = 0;
     delay(400);
    }
  }
  for(int k = 0; k < 5; k++)
    {
      d += m[k] / 5;
    }
  v = d * a;
  v = v / 1000;
  return v;
}

//FUNCAO QUE PARA AS DUAS BOMBAS
void pararRegar()
{
        wifi(distancia, distancia2, umidadeSolo, 2);
        digitalWrite(porta_rele1, HIGH);
        digitalWrite(porta_rele2, HIGH);
        delay(3000);
}

void setup() 
{
  //DEFINE O AS PORTAS DOS RELÉS OBS: "porta_rele1" É A AGUA DA CHUVA
  pinMode(porta_rele1, OUTPUT);
  pinMode(porta_rele2, OUTPUT);
  pararRegar();
  //WI-FÉ
  Serial.begin(115200);
  esp8266.begin(115200);
 
  sendData("AT+RST\r\n", 2000, DEBUG); // rst
  // Conecta a rede wireless
  sendData("AT+CWSAP=\"IRRIGACAO\",\"daesp-eco\",5,3\r\n", 2000, DEBUG);
  delay(3000);
  sendData("AT+CWMODE=2\r\n", 1000, DEBUG);
  // Mostra o endereco IP
  sendData("AT+CIFSR\r\n", 1000, DEBUG);
  // Configura para multiplas conexoes
  sendData("AT+CIPMUX=1\r\n", 1000, DEBUG);
  // Inicia o web server na porta 80
  sendData("AT+CIPSERVER=1,80\r\n", 1000, DEBUG);          
}

void loop() 
{  
  //GARANTE QUE AS BOMBAS ESTAO DESLIGADAS
  digitalWrite(porta_rele2, HIGH);
  digitalWrite(porta_rele1, HIGH); 
  
  delay(5000);
  //MEDIR O VOLUME DA AGUA
  distancia = volume(1,h,a);
  distancia2 = volume(2,h,a);
  
  //CHECAR UMIDADE DO SOLO 5x EM 5s
  for(int i=0; i < 5; i++) 
  {
    umidadeSolo = analogRead(pinoSensor);
    umidadeSolo = map(umidadeSolo, 1023, 0, 0, 150);

    if(umidadeSolo >=100)
      umidadeSolo = 100;
    delay(1000); 
  }
  wifi(distancia, distancia2, umidadeSolo, 0);
  delay(7000);
  //REGAR COM AGUA DA CHUVA
  if (distancia2 >= 0.22)
    {
      if(umidadeSolo < limiarSeco)
      {
        wifi(distancia, distancia2, umidadeSolo, 1);
        digitalWrite(porta_rele1, LOW);
        digitalWrite(porta_rele2, HIGH);
        
        delay(tempoRega*1000);
      }
      
      //PARAR DE REGAR
      else
      {
        pararRegar();
      }                          
    
    }
  else
  {
      //REGAR COM AGUA DO TANQUE COMUM
      if(umidadeSolo < limiarSeco)
      {
        wifi(distancia, distancia2, umidadeSolo, 1);
        digitalWrite(porta_rele1, HIGH);
        digitalWrite(porta_rele2, LOW);
        
        delay(tempoRega*1000);
      }
      //PARAR DE REGAR
      else 
      {
        pararRegar();
      }
  }
}
