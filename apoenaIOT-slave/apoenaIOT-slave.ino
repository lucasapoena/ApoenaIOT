#include <ESP8266WiFi.h>  //https://github.com/esp8266/Arduino
#include <DNSServer.h>  //Servidor DNS local usado para redirecionar todas as solicitações para o portal de configuração
#include <ESP8266WebServer.h> //Biblioteca do WebServer
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager

ESP8266WebServer  webServer(80); // Inicializa a biblioteca do Web Server na porta HTTP (80)
WiFiManager       wifiManager; // Inicializa a biblioteca do WiFiManager 

/* Utilizado o PROGMEM para habilitar o armazenarnamento das constantes na Flash e economizar espaço na RAM */
#define PROGMEM ICACHE_RODATA_ATTR

// Dados de acesso a central de controle slave
const char AP_SSID_NAME[] = "ApoenaIOT";
const char AP_PASSWORD[]  = "password";

/*
Segue abaixo a equivalencia das saidas Digitais entre nodeMCU e ESP8266 (na IDE do Arduino)
D0 = 16;
D1 = 5;
D2 = 4;
D3 = 0;
D4 = 2;
D5 = 14;
D6 = 12;
D7 = 13;
D8 = 15;
D9 = 3;
D10 = 1;
*/

const uint8_t GPIOPIN_CARGAS[3] = {D1,D2,D3}; // Pinos mapeados para acionamento de cargas
float valorTemperatura = 0 ;
float valorUmidade = 0 ;
String statusGpioCargas[3] = {"OFF","OFF","OFF"};

void setup(void) {
  Serial.begin(115200); // Inicializa o monitor serial
  // Configuração dos pinos de acionamento de cargas elétricas
  for ( int x = 0 ; x <= 3 ; x++ ) { 
    pinMode(GPIOPIN_CARGAS[x],OUTPUT);
  }

  Serial.println(ESP.getBootMode());
  Serial.print(F("ESP.getSdkVersion(); "));
  Serial.println(ESP.getSdkVersion());
  Serial.print("ESP.getBootVersion(); ");
  Serial.println(ESP.getBootVersion());
  Serial.print("ESP.getChipId(); ");
  Serial.println(ESP.getChipId());
  Serial.print("ESP.getFlashChipSize(); ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("ESP.getFlashChipRealSize(); ");
  Serial.println(ESP.getFlashChipRealSize());
  Serial.print("ESP.getFlashChipSizeByChipId(); ");
  Serial.println(ESP.getFlashChipSizeByChipId());
  Serial.print("ESP.getFlashChipId(); ");
  Serial.println(ESP.getFlashChipId());
  configuraAP();
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

void loop(void) {
  webServer.handleClient();  
}

void configuraAP(){  
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  wifiManager.setConfigPortalTimeout(180);
  // Caso não consiga se conectar na rede configurada, volta para o modo AP com o SSID: ApoenaIOT
  if(!wifiManager.autoConnect(AP_SSID_NAME, AP_PASSWORD)) {
    Serial.println(F("Tempo limite de tentativas de conexão esgotado"));
    restartDevice();
  }else{
    Serial.println(F("Conexao Estabelecida..."));
    inicializaWebServer();
  }  
}

void restartDevice(){
    Serial.println(F("Realizando o reboot do equipamento..."));
    delay(3000); 
    ESP.reset();
    delay(5000); 
}

void getResetConfig(){
  char html[400]; 
  snprintf (html, 400,
    "<html>\
      <head>\
        <title>Reset - Config</title>\
      </head>\
      <body>\
        <h1>Restaurando configigurações de Fábrica</h1>\
        <form method='POST'>\
          <p><input type='submit' value='Reset' /></p>\
        </form>\
      </body>\
    </html>"
  );  
  webServer.send ( 200, "text/html", html);  
}

void postResetConfig(){
  Serial.println("Restaurando configurações de fábrica...");
  wifiManager.resetSettings();
  restartDevice();
}

void inicializaWebServer(){
  webServer.on("/info", [](){
    webServer.send(200, "text/plain", "Informacoes");
  });
  webServer.on("/restart", restartDevice);
  webServer.on("/resetConfig", HTTP_GET, getResetConfig);
  webServer.on("/resetConfig", HTTP_POST, postResetConfig);
  webServer.on ( "/config.html", handleRoot );
  webServer.on ( "/sensores.html", handleRoot );
  webServer.on ( "/iluminacao.html", handleRoot );
  webServer.on ( "/index.html", handleRoot );
  webServer.on ( "/", handleRoot );
  webServer.onNotFound(handleNotFound);  
  webServer.begin();
  Serial.println ( "HTTP server started" );
}


// Método para gerar a estrutura do template padrão
String getTemplatePagina(String conteudoHtml, String tituloPagina){
  String templatePagina = "";
  if (tituloPagina == "") tituloPagina = "ApoenaIOT";  
  
  templatePagina += "<!DOCTYPE html> <html lang='pt-br'> <head> <meta http-equiv='refresh' content='30' name='viewport' content='width=device-width, initial-scale=1'/> <meta charset='utf-8'> <meta http-equiv='X-UA-Compatible' content='IE=edge'> <meta name='description' content='ApoenaIOT'> <meta name='author' content='Lucas Apoena'> <link href='https://cdn.rawgit.com/lucasapoena/apoenaIOT-slave/master/template/css/css.css' rel='stylesheet'> <title>ApoenaIOT - Central Slave</title> </head> <body> <div id='wrapper'> <nav class='navbar navbar-default navbar-static-top' role='navigation' style='margin-bottom: 0'> <div class='navbar-header'> <button type='button' class='navbar-toggle' data-toggle='collapse' data-target='.navbar-collapse'> <span class='sr-only'>Menu</span> <span class='icon-bar'></span> <span class='icon-bar'></span> <span class='icon-bar'></span> </button> <a class='navbar-brand' href='index.html'>ApoenaIOT - Central Slave</a> </div> <ul class='nav navbar-top-links navbar-right'> <li class='dropdown'> <a class='dropdown-toggle' data-toggle='dropdown' href='#'> <i class='fa fa-tasks fa-fw'></i> <i class='fa fa-caret-down'></i> </a> <ul class='dropdown-menu dropdown-tasks'> <li> <a href='#'> <div> <p> <strong>CPU</strong> <span class='pull-right text-muted'>40% Usado</span> </p> <div class='progress progress-striped active'> <div class='progress-bar progress-bar-success' role='progressbar' aria-valuenow='40' aria-valuemin='0' aria-valuemax='100' style='width: 40%'> <span class='sr-only'>40% Complete (success)</span> </div> </div> </div> </a> </li> <li class='divider'></li> <li> <a href='#'> <div> <p> <strong>Memória</strong> <span class='pull-right text-muted'>80% Usada</span> </p> <div class='progress progress-striped active'> <div class='progress-bar progress-bar-danger' role='progressbar' aria-valuenow='80' aria-valuemin='0' aria-valuemax='100' style='width: 80%'> <span class='sr-only'>80% Complete (danger)</span> </div> </div> </div> </a> </li> <li class='divider'></li> <li> <a href='#'> <div> <p> <strong>Armazenamento</strong> <span class='pull-right text-muted'>20% Usada</span> </p> <div class='progress progress-striped active'> <div class='progress-bar progress-bar-info' role='progressbar' aria-valuenow='20' aria-valuemin='0' aria-valuemax='100' style='width: 20%'> <span class='sr-only'>20% Complete</span> </div> </div> </div> </a> </li> </ul> </li> </ul> <div class='navbar-default sidebar' role='navigation'> <div class='sidebar-nav navbar-collapse'> <ul class='nav' id='side-menu'> <li> <a href='index.html'><i class='fa fa-dashboard fa-fw'></i> Dashboard</a> </li> <li> <a href='#'><i class='fa fa-sitemap fa-fw'></i> Dispositivos<span class='fa arrow'></span></a> <ul class='nav nav-second-level'> <li> <a href='iluminacao.html'><i class='fa fa-lightbulb-o' aria-hidden='true'></i> Iluminação</a> </li> <li> <a href='sensores.html'><i class='fa fa-bar-chart-o fa-fw'></i> Sensores</a> </li> </ul> </li> <li> <a href='config.html'><i class='fa fa-wrench fa-fw'></i> Configurações</a> </li> </ul> </div> </div> </nav> <div id='page-wrapper'> <div class='container-fluid'> <div class='row'><div class='col-lg-12'><h1 class='page-header'>"+tituloPagina+"</h1></div></div> <div class='row'>";
  
  // ---- Inicio - Conteudo -----
  templatePagina += conteudoHtml;
  // ---- Final - Conteudo -----
  
  templatePagina += "</div> </div> </div> </div> <script src='https://cdn.rawgit.com/lucasapoena/apoenaIOT-slave/master/template/js/js.js'></script> </body> </html>";
  
  //"<html lang='pt-br'> <head> <meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1'/> <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0-alpha.6/css/bootstrap.min.css' integrity='sha384-rwoIResjU2yc3z8GV/NPeZWAv56rSmLldC3R/AZzGRnGxQQKnKkoFVhFQhNUwEyJ' crossorigin='anonymous'> <script src='https://code.jquery.com/jquery-3.1.1.slim.min.js' integrity='sha384-A7FZj7v+d/sdmMqp/nOQwliLvUsJfDHW+k9Omg/a/EheAdgtzNs3hpfag6Ed950n' crossorigin='anonymous'></script> <script src='https://cdnjs.cloudflare.com/ajax/libs/tether/1.4.0/js/tether.min.js' integrity='sha384-DztdAPBWPRXSA/3eYEEUWrWCy7G5KFbe8fFjk5JAIxUYHKkDx6Qin1DkWx51bBrb' crossorigin='anonymous'></script> <script src='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0-alpha.6/js/bootstrap.min.js' integrity='sha384-vBWWzlZJ8ea9aCX4pEW3rVHjgjt7zpkNpZk+02D9phzyeVkE+jo0ieGizqPLForn' crossorigin='anonymous'></script> <title>ApoenaIOT</title> </head> <body> <div class='container-fluid'> <div class='row'> <div class='col-md-12'> <h1>ApoenaIOT</h1> <h3>Central Slave - Cod.:0000</h3> <table class='table'> <thead> <tr> <th>-</th> <th>Descrição</th> </tr> </thead> <tbody> <tr> <td>SSID</td> <td>ApoenaIOT</td> </tr> <tr> <td>IP</td> <td>192.168.1.1</td> </tr> <tr> <td>Ambiente</td> <td>Sala de estar</td> </tr> </tbody> </table> <h3>Equipamentos</h3> <div class='row'> <div class='col-md-4'> <h4 class ='text-left'>Lampada principal <span class='badge'>"+ statusGpioCargas[0];
  //"</span> </h4> </div> <div class='col-md-4'><form action='/' method='POST'><button type='button submit' name='D1' value='1' class='btn btn-success btn-lg'>ON</button></form></div> <div class='col-md-4'><form action='/' method='POST'><button type='button submit' name='D1' value='0' class='btn btn-danger btn-lg'>OFF</button></form></div> </div> <div class='row'> <div class='col-md-4'> <h4 class ='text-left'>Ventilador <span class='badge'> statusGpioCargas[1] </span> </h4> </div> <div class='col-md-4'><form action='/' method='POST'><button type='button submit' name='D2' value='1' class='btn btn-success btn-lg'>ON</button></form></div> <div class='col-md-4'><form action='/' method='POST'><button type='button submit' name='D2' value='0' class='btn btn-danger btn-lg'>OFF</button></form></div> </div> </div> </div> </div> </body> </html>";    
   return templatePagina;
}

void handleRoot(){
    String conteudo = "<div class='row'><div class='col-lg-3 col-md-6'><div class='panel panel-green'><div class='panel-heading'><div class='row'><div class='col-xs-3'><i class='fa fa-bolt fa-5x'></i></div><div class='col-xs-9 text-right'><div class='huge'>02</div> <div>Ligados</div> </div> </div> </div> <a href='#'> <div class='panel-footer'> <span class='pull-left'>Visualizar...</span> <span class='pull-right'><i class='fa fa-arrow-circle-right'></i></span> <div class='clearfix'></div> </div> </a> </div> </div> <div class='col-lg-3 col-md-6'> <div class='panel panel-red'> <div class='panel-heading'> <div class='row'> <div class='col-xs-3'> <i class='fa fa-power-off fa-5x'></i> </div> <div class='col-xs-9 text-right'> <div class='huge'>01</div> <div>Desligados</div> </div> </div> </div> <a href='#'> <div class='panel-footer'> <span class='pull-left'>Visualizar...</span> <span class='pull-right'><i class='fa fa-arrow-circle-right'></i></span> <div class='clearfix'></div> </div> </a> </div> </div> </div> <div class='row'> <div class='col-lg-12'> <div class='panel panel-default'> <div class='panel-heading'> Informações da Placa </div> <div class='panel-body'> <div class='table-responsive'> <table class='table table-striped table-bordered table-hover'> <thead> <tr> <th>#</th> <th>Descrição</th> </tr> </thead> <tbody> <tr> <td>Sistema</td> <td>ApoenaIOT-Slave v0.0.1</td> </tr> <tr> <td>Modelo da placa</td> <td>NodeMCU v3 Lolin</td> </tr> <tr> <td>Serial</td> <td>15as15dd00das5a4</td> </tr> <tr> <td>SSID</td> <td>"+String(WiFi.SSID())+"</td> </tr> <tr> <td>IP</td> <td>"+WiFi.localIP().toString()+"</td> </tr> <tr> <td>Subnet</td> <td>"+WiFi.subnetMask().toString()+"</td> </tr> <tr> <td>Gateway</td> <td>"+WiFi.gatewayIP().toString()+"</td> </tr> </tbody> </table> </div> </div> </div> </div> </div> ";
    if ( webServer.hasArg("D1") ) {
      handleD1();
    } else if ( webServer.hasArg("D2") ) {
      handleD2();
    } else {
      webServer.send ( 200, "text/html", getTemplatePagina(conteudo,"Dashboard") );
    }
}
void handleNotFound(){
  webServer.send(404, "text/plain", "404: Not found");
}
void handleD1() {
  updateGPIO(0,webServer.arg("D1")); 
}
 
void handleD2() {
  updateGPIO(1,webServer.arg("D2")); 
}
 
 
void updateGPIO(int gpio, String DxValue) {
  Serial.println("");
  Serial.println("Update GPIO "); Serial.print(GPIOPIN_CARGAS[gpio]); Serial.print(" -> "); Serial.println(DxValue);
  
  if ( DxValue == "1" ) {
    digitalWrite(GPIOPIN_CARGAS[gpio], HIGH);
    statusGpioCargas[gpio] = "On";
    webServer.send ( 200, "text/html", getTemplatePagina("","") );
  } else if ( DxValue == "0" ) {
    digitalWrite(GPIOPIN_CARGAS[gpio], LOW);
    statusGpioCargas[gpio] = "Off";
    webServer.send ( 200, "text/html", getTemplatePagina("","") );
  } else {
    Serial.println("Err Led Value");
  }  
}


