#include <ESP8266WiFi.h>  //https://github.com/esp8266/Arduino
#include <DNSServer.h>  //Servidor DNS local usado para redirecionar todas as solicitações para o portal de configuração
#include <ESP8266WebServer.h> //Biblioteca do WebServer
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager

ESP8266WebServer  webServer(80); // Inicializa a biblioteca do Web Server na porta HTTP (80)
WiFiManager       wifiManager; // Inicializa a biblioteca do WiFiManager
DNSServer         dnsServer; 

/* Utilizado o PROGMEM para habilitar o armazenarnamento das constantes na Flash e economizar espaço na RAM */
#define PROGMEM ICACHE_RODATA_ATTR
#define QTD_RELES 3

// Dados de acesso a central de controle slave
char * AP_SSID_NAME = "ApoenaIOT_";
const char * AP_PASSWORD  = "password";
/*
**  Segue abaixo a equivalencia das saidas Digitais entre nodeMCU e ESP8266 (na IDE do Arduino)
**  D0 = 16; D1 = 5; D2 = 4; D3 = 0; D4 = 2; D5 = 14; D6 = 12; D7 = 13; D8 = 15; D9 = 3; D10 = 1;
*/
const uint8_t GPIOPIN_CARGAS[QTD_RELES] = {D1,D2,D3}; // Pinos mapeados para acionamento de cargas
float valorTemperatura = 0 ;
float valorUmidade = 0 ;
byte statusGpioCargas[QTD_RELES] = {0,0,1};

void setup(void) {
  Serial.begin(115200); // Inicializa o monitor serial
  
  // Configuração dos pinos de acionamento de cargas elétricas
  for ( int x = 0 ; x < QTD_RELES ; x++ ) { 
    pinMode(GPIOPIN_CARGAS[x],OUTPUT);
  }
  
  configuraAP();
  
  Serial.println('\n');
  Serial.println("----------------- Informacoes - ESP: -----------------");
  Serial.print(F("ESP.getSdkVersion(); "));
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
  Serial.println("----------------- Informacoes - Conexao: -----------------");
  Serial.println("Conectado a "+WiFi.SSID());
  Serial.println("IP address: "+WiFi.localIP().toString());
  Serial.println("Subnet: "+WiFi.subnetMask().toString());
  Serial.println("Gateway: "+WiFi.gatewayIP().toString());
}

void loop(void) {
  webServer.handleClient();  
}

void configuraAP(){  
  Serial.println(F("Iniciando configuração do AP:"));
  char charBuf[50];
  WiFi.hostname().toCharArray(charBuf, 50);
  strcat(AP_SSID_NAME, charBuf);
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  wifiManager.setConfigPortalTimeout(180);
  // Caso não consiga se conectar na rede configurada, volta para o modo AP com o SSID: ApoenaIOT
  if(!wifiManager.autoConnect(AP_SSID_NAME, AP_PASSWORD)) {
    Serial.println(F("Tempo limite de tentativas de conexão esgotado"));
    resetDevice();
  }else{
    Serial.println(F("Conexao Estabelecida..."));
    inicializaWebServer();
  }
}

void resetDevice(){
    Serial.println(F("Realizando o reboot do equipamento..."));
    delay(3000); 
    ESP.reset();
    delay(5000); 
}
void restartDevice(){
    Serial.println(F("Realizando o reboot do equipamento..."));
    delay(3000); 
    ESP.restart();
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

void getRestaurarConfiguracao(){
  Serial.println("Restaurando configurações de fábrica...");
  wifiManager.resetSettings();
  configuraAP();
}

void inicializaWebServer(){
  webServer.on("/restart_equipamento.html", HTTP_GET, templateRestartEquipamento);
  webServer.on("/restart_equipamento.html", HTTP_POST, restartDevice);
  webServer.on ("/restauracao_configuracao.html", HTTP_GET, templateRestaurarConfiguracao);  
  webServer.on("/restauracao_configuracao.html", HTTP_POST, getRestaurarConfiguracao);
  webServer.on ( "/sensores.html", templateSensores );
  webServer.on ( "/iluminacao.html", templateIluminacao);
  webServer.on ( "/index.html", templateDashboard );
  webServer.on ( "/", templateDashboard );
  webServer.onNotFound(handleNotFound);  
  webServer.begin();
  Serial.println ( "HTTP server started" );
}


// Método para gerar a estrutura do template padrão
String getTemplatePagina(String conteudoHtml, String tituloPagina){
  String templatePagina = "";
  if (tituloPagina == "") tituloPagina = "ApoenaIOT";  
  templatePagina += "<!DOCTYPE html><html lang='pt-br'><head><meta charset='utf-8'><meta http-equiv='X-UA-Compatible' content='IE=edge'><meta name='viewport' content='width=device-width, initial-scale=1'><meta charset='utf-8'><meta http-equiv='X-UA-Compatible' content='IE=edge'><meta name='description' content='ApoenaIOT'><meta name='author' content='Lucas Apoena'><link href='https://cdn.rawgit.com/lucasapoena/apoenaIOT-slave/master/template/css/css.css' rel='stylesheet'><title>ApoenaIOT - Central Slave</title></head><body><div id='wrapper'><nav class='navbar navbar-default navbar-static-top' role='navigation' style='margin-bottom: 0'><div class='navbar-header'><button type='button' class='navbar-toggle' data-toggle='collapse' data-target='.navbar-collapse'><span class='sr-only'>Menu</span><span class='icon-bar'></span><span class='icon-bar'></span><span class='icon-bar'></span></button><a class='navbar-brand' href='index.html'>ApoenaIOT - Central Slave</a></div><ul class='nav navbar-top-links navbar-right'><li class='dropdown'><a class='dropdown-toggle' data-toggle='dropdown' href='#'><i class='fa fa-tasks fa-fw'></i><i class='fa fa-caret-down'></i></a><ul class='dropdown-menu dropdown-messages'><li><a href='#'><div><p><strong>CPU</strong><span class='pull-right text-muted'>40% Usado</span></p><div class='progress progress-striped active'><div class='progress-bar progress-bar-success' role='progressbar' aria-valuenow='40' aria-valuemin='0' aria-valuemax='100' style='width: 40%'><span class='sr-only'>40% Complete (success)</span></div></div></div></a></li><li class='divider'></li><li><a href='#'><div><p><strong>Memória</strong><span class='pull-right text-muted'>80% Usada</span></p><div class='progress progress-striped active'><div class='progress-bar progress-bar-danger' role='progressbar' aria-valuenow='80' aria-valuemin='0' aria-valuemax='100' style='width: 80%'><span class='sr-only'>80% Complete (danger)</span></div></div></div></a></li><li class='divider'></li><li><a href='#'><div><p><strong>Armazenamento</strong><span class='pull-right text-muted'>20% Usada</span></p><div class='progress progress-striped active'><div class='progress-bar progress-bar-info' role='progressbar' aria-valuenow='20' aria-valuemin='0' aria-valuemax='100' style='width: 20%'><span class='sr-only'>20% Complete</span></div></div></div></a></li></ul></li></ul><div class='navbar-default sidebar' role='navigation'><div class='sidebar-nav navbar-collapse'><ul class='nav' id='side-menu'><li><a href='index.html'><i class='fa fa-dashboard fa-fw'></i> Dashboard</a></li><li><a href='#'><i class='fa fa-sitemap fa-fw'></i> Dispositivos<span class='fa arrow'></span></a><ul class='nav nav-second-level'><li><a href='iluminacao.html'><i class='fa fa-lightbulb-o' aria-hidden='true'></i> Iluminação</a></li><li><a href='sensores.html'><i class='fa fa-bar-chart-o fa-fw'></i> Sensores</a></li></ul></li><li><a href='#'><i class='fa fa-wrench fa-fw'></i> Configurações<span class='fa arrow'></span></a><ul class='nav nav-second-level'><li><a href='restauracao_configuracao.html'>Restaurar Configurações</a></li><li><a href='restart_equipamento.html'>Reboot</a></li></ul></li></ul></div></div></nav><div id='page-wrapper'><div class='container-fluid'><div class='row'><div class='col-lg-12'><h1 class='page-header'>"+tituloPagina+"</h1></div></div><div class='row'> "+conteudoHtml+"</div></div></div></div><script src='https://cdn.rawgit.com/lucasapoena/apoenaIOT-slave/master/template/js/js.js'></script></body></html> ";
  return templatePagina;
}

void templateDashboard(){
  String conteudoHtml = "";
  int dispositivosLigados = 0;
  int dispositivosDesligados = 0;  
  for (int i=0;i<QTD_RELES;i++){ 
    dispositivosLigados += int(statusGpioCargas[i]);    
  }
  dispositivosDesligados = QTD_RELES - dispositivosLigados;
  conteudoHtml += "<div class='row'><div class='col-lg-3 col-md-6'><div class='panel panel-green'><div class='panel-heading'><div class='row'><div class='col-xs-3'><i class='fa fa-bolt fa-5x'></i></div><div class='col-xs-9 text-right'><div class='huge'>"+ String(dispositivosLigados) +"</div><div>Ligados</div></div></div></div><a href='iluminacao.html'><div class='panel-footer'><span class='pull-left'>Visualizar...</span><span class='pull-right'><i class='fa fa-arrow-circle-right'></i></span><div class='clearfix'></div></div></a></div></div><div class='col-lg-3 col-md-6'><div class='panel panel-red'><div class='panel-heading'><div class='row'><div class='col-xs-3'><i class='fa fa-power-off fa-5x'></i></div><div class='col-xs-9 text-right'><div class='huge'>"+ String(dispositivosDesligados) +"</div><div>Desligados</div></div></div></div><a href='iluminacao.html'><div class='panel-footer'><span class='pull-left'>Visualizar...</span><span class='pull-right'><i class='fa fa-arrow-circle-right'></i></span><div class='clearfix'></div></div></a></div></div></div><div class='row'><div class='col-lg-12'><div class='panel panel-default'><div class='panel-heading'> Informações da Placa</div><div class='panel-body'><div class='table-responsive'><table class='table table-striped table-bordered table-hover'><thead><tr><th>#</th><th>Descrição</th></tr></thead><tbody><tr><td>Sistema</td><td>ApoenaIOT-Slave v0.0.1</td></tr><tr><td>Modelo da placa</td><td>NodeMCU v3 Lolin</td></tr><tr><td>Serial</td><td>15as15dd00das5a4</td></tr><tr><td>Hostname</td><td>"+WiFi.hostname()+"</td></tr><tr><td>SSID</td><td>"+WiFi.SSID()+"</td></tr><tr><td>IP</td><td>"+WiFi.localIP().toString()+"</td></tr><tr><td>Subnet</td><td>"+WiFi.subnetMask().toString()+"</td></tr><tr><td>Gateway</td><td>"+WiFi.gatewayIP().toString()+"</td></tr></tbody></table></div></div></div></div></div>";
  //conteudoHtml += "<div class='row'><div class='col-lg-3 col-md-6'><div class='panel panel-green'><div class='panel-heading'><div class='row'><div class='col-xs-3'><i class='fa fa-bolt fa-5x'></i></div><div class='col-xs-9 text-right'><div class='huge'>"+ String(dispositivosLigados) +"</div><div>Ligados</div></div></div></div><a href='#'><div class='panel-footer'><span class='pull-left'>Visualizar...</span><span class='pull-right'><i class='fa fa-arrow-circle-right'></i></span><div class='clearfix'></div></div></a></div></div><div class='col-lg-3 col-md-6'><div class='panel panel-red'><div class='panel-heading'><div class='row'><div class='col-xs-3'><i class='fa fa-power-off fa-5x'></i></div><div class='col-xs-9 text-right'><div class='huge'>"+ String(dispositivosDesligados) +"</div><div>Desligados</div></div></div></div><a href='#'><div class='panel-footer'><span class='pull-left'>Visualizar...</span><span class='pull-right'><i class='fa fa-arrow-circle-right'></i></span><div class='clearfix'></div></div></a></div></div></div><div class='row'><div class='col-lg-12'><div class='panel panel-default'><div class='panel-heading'>Informações da Placa</div><div class='panel-body'><div class='table-responsive'><table class='table table-striped table-bordered table-hover'><thead><tr><th>#</th><th>Descrição</th></tr></thead><tbody><tr><td>Sistema</td><td>ApoenaIOT-Slave v0.0.1</td></tr><tr><td>Modelo da placa</td><td>NodeMCU v3 Lolin</td></tr><tr><td>Serial</td><td>15as15dd00das5a4</td></tr><tr><td>SSID</td><td>"+WiFi.SSID()+"</td></tr><tr><td>IP</td><td>"+WiFi.localIP().toString()+"</td></tr><tr><td>Subnet</td><td>"+WiFi.subnetMask().toString()+"</td></tr><tr><td>Gateway</td><td>"+WiFi.gatewayIP().toString()+"</td></tr></tbody></table></div></div></div></div></div>";
  webServer.send ( 200, "text/html", getTemplatePagina(conteudoHtml,"Dashboard") );
}
void templateIluminacao(){
  String nomesDispositivos[QTD_RELES] = {"Luz Principal","Abajur - Star Wars","Fita de Led"};
  String conteudoHtml = "";
  conteudoHtml += "<div class='row'>";
  for (int i=0; i<QTD_RELES; i++){
    if (webServer.hasArg("button_"+String(i))){
      atualizaStatusRele(i, webServer.arg("button_"+String(i)));
    }
    conteudoHtml += "<div class='col-lg-3'>";
    if(statusGpioCargas[i]){ 
      // Caso Ligado
      conteudoHtml += "<div class='panel panel-green'><div class='panel-heading'><i class='fa fa-bolt'></i> (Ligado) Lâmpada - Cod.: D"+String(i)+"</div><div class='panel-body'><form action='iluminacao.html' method='POST'><button type='button submit' name='button_"+String(i)+"' value='0' class='btn btn-danger btn-lg' ><i class='fa fa-power-off'></i> <br/>Desligar</button></form></div><div class='panel-footer'>"+nomesDispositivos[i]+"</div></div>";
    }else{
      // Caso desligado
       conteudoHtml += "<div class='panel panel-red'><div class='panel-heading'><i class='fa fa-power-off'></i> (Desligado) Lâmpada - Cod.: D"+String(i)+"</div><div class='panel-body'><form action='iluminacao.html' method='POST'><button type='button submit' name='button_"+String(i)+"' value='1' class='btn btn-success btn-lg'><i class='fa fa-bolt'></i><br/>Ligar</button></form></div><div class='panel-footer'>"+nomesDispositivos[i]+"</div></div>";
    }    
    conteudoHtml += "</div>";
  }
  conteudoHtml += "</div>";
  
  webServer.send ( 200, "text/html", getTemplatePagina(conteudoHtml,"Iluminação") );
}

void templateSensores(){
  String conteudoHtml = "";
  conteudoHtml +="";
   webServer.send ( 200, "text/html", getTemplatePagina(conteudoHtml,"Sensores") );
}

void templateRestaurarConfiguracao(){
  String conteudoHtml = "";
  conteudoHtml +="<div class='col-lg-12'><div class='panel panel-default'><div class='panel-heading'> Restaurar Configurações de Fábrica</div><div class='panel-body'><p>Clique no botão abaixo para realizar a restauração de fábrica da sua Apoena - Cetral Slave</p><form action='restauracao_configuracao.html' method='POST'><button type='button submit' onclick='confirm();' class='btn btn-default'>Restore</button></form></div><div class='panel-footer'></div></div></div>";
  webServer.send ( 200, "text/html", getTemplatePagina(conteudoHtml,"Restaurar") );
}

void templateRestartEquipamento(){
  String conteudoHtml = "";
  conteudoHtml +="<div class='col-lg-12'><div class='panel panel-default'><div class='panel-heading'> Reboot - Equipamento</div><div class='panel-body'><p>Clique no botão abaixo para realizar o reboot da sua Apoena - Cetral Slave</p><form action='restart_equipamento.html' method='POST'><button type='button submit' onclick='confirm();' class='btn btn-default'>Reboot</button></form></div><div class='panel-footer'></div></div></div>";
  webServer.send ( 200, "text/html", getTemplatePagina(conteudoHtml,"Restaurar") );
}

void atualizaStatusRele(int pinoGPIO, String valorButton ){
  Serial.println("---------------------- Atualizando status do pino -----------------");
  Serial.println("Atualizando o pino:"+ String(GPIOPIN_CARGAS[pinoGPIO]) +" para:"+valorButton);
  if (valorButton == "0"){
    digitalWrite(GPIOPIN_CARGAS[pinoGPIO], LOW);
    statusGpioCargas[pinoGPIO] = 0;
  }else if (valorButton == "1"){
    digitalWrite(GPIOPIN_CARGAS[pinoGPIO], HIGH);
    statusGpioCargas[pinoGPIO] = 1;
  }else{
    Serial.println(F("Valor de referência invalido"));
  }
  
}

void handleNotFound(){
  webServer.send (404, "text/html", getTemplatePagina("Página não encontrada","Erro 404") );
}




