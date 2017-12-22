/*
 * 
 * ApoenaIOT-slave v0.0.1
 *
 * Controle e automação de dispositivos.  
 * @Autor: Lucas Apoena  
 * @E-mail: lucasapena@gmail.com  
 * https://github.com/lucasapoena/apoenaIOT-slave
 * 
 */
 
#include <ESP8266WiFi.h>  //https://github.com/esp8266/Arduino
#include <DNSServer.h>  //Servidor DNS local usado para redirecionar todas as solicitações para o portal de configuração
#include <ESP8266WebServer.h> //Biblioteca do WebServer
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager

ESP8266WebServer  webServer(80); // Inicializa a biblioteca do Web Server na porta HTTP (80)
WiFiManager       wifiManager; // Inicializa a biblioteca do WiFiManager

#define QTD_RELES 3
String versaoApp = "v0.0.1";
char * AP_NOME_SSID = "ApoenaIOT_";
char * AP_SSID_PASSWORD  = "password";

/*
**  Segue abaixo a equivalencia das saidas Digitais entre nodeMCU e ESP8266 (na IDE do Arduino)
**  D0 = 16; D1 = 5; D2 = 4; D3 = 0; D4 = 2; D5 = 14; D6 = 12; D7 = 13; D8 = 15; D9 = 3; D10 = 1;
*/
const uint8_t GPIOPIN_RELES[] = {D1,D2,D3}; // Pinos mapeados para acionamento de cargas
uint8_t statusGpioReles[QTD_RELES]; 
float valorTemperatura = 0 ;
float valorUmidade = 0 ;

void setup(void) {
  Serial.begin(115200); // Inicializa o monitor serial
  
  // Configuração dos pinos de acionamento de cargas elétricas
  for (int x = 0 ; x < QTD_RELES ; x++ ) { 
    pinMode(GPIOPIN_RELES[x],OUTPUT); // Configura pino como saída para acionamento dos relés
    statusGpioReles[x] = 0; // Configura o status inicial dos relés como desligados    
   
    Serial.println('\n');
    Serial.println("------------------ Status - Relés ------------------");
    Serial.print("GPIOPIN:");
    Serial.print(GPIOPIN_RELES[x]);
    Serial.print(" - Status:");
    Serial.println(statusGpioReles[x]);
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

// ------------------------------------ Métodos de Controle ------------------------------ 
void configuraAP(){
  Serial.println("---------------------- Configuração - AP -----------------");  
  Serial.println(F("Iniciando configuração do AP:"));

  // Procedimento para concatenar o hostname da placa com o nome SSID padrão
  char hostnameBufer[50];
  WiFi.hostname().toCharArray(hostnameBufer, 50);
  strcat(AP_NOME_SSID, hostnameBufer);
  
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  wifiManager.setConfigPortalTimeout(180);
  
  // Caso não consiga se conectar na rede configurada, volta para o modo AP com o SSID: ApoenaIOT
  if(!wifiManager.autoConnect(AP_NOME_SSID, AP_SSID_PASSWORD)) {
    Serial.println(F("Tempo limite de tentativas de conexão esgotado"));
    resetDevice();
  }else{
    Serial.println(F("Conexao Estabelecida..."));
    inicializaWebServer();
  }

}

void resetDevice(){
    Serial.println("---------------------- Configuração - Reset -----------------");
    Serial.println(F("Realizando o hard reset do equipamento..."));
    delay(3000); 
    ESP.reset();
    delay(5000); 
}
void restartDevice(){
    Serial.println("---------------------- Configuração - Reboot -----------------");
    Serial.println(F("Realizando o reboot do equipamento..."));
    delay(3000); 
    ESP.restart();
    delay(5000); 
}
void restaurarConfiguracaoFabrica(){
  Serial.println("---------------------- Configuração - Restaurar -----------------");
  Serial.println("Restaurando configurações de fábrica...");
  wifiManager.resetSettings();
  configuraAP();
}

void atualizaStatusRele(uint8_t pinoGPIO, String valorButton ){
  Serial.println("---------------------- Atualizando status do pino -----------------");
  Serial.println("Atualizando o pino:"+ String(GPIOPIN_RELES[pinoGPIO]) +" para:"+valorButton);
  if (valorButton == "0"){
    if (pinoGPIO == 0){digitalWrite(D1, LOW);}
    else if (pinoGPIO == 1){digitalWrite(D2, LOW);}
    else if (pinoGPIO == 2){digitalWrite(D3, LOW);}
    digitalWrite(GPIOPIN_RELES[pinoGPIO], LOW);
    statusGpioReles[pinoGPIO] = 0;
  }else if (valorButton == "1"){
    if (pinoGPIO == 0){digitalWrite(D1, HIGH);}
    else if (pinoGPIO == 1){digitalWrite(D2, HIGH);}
    else if (pinoGPIO == 2){digitalWrite(D3, HIGH);}
    digitalWrite(GPIOPIN_RELES[pinoGPIO], HIGH);
    statusGpioReles[pinoGPIO] = 1;
  }else{
    Serial.println(F("Valor de referência invalido"));
  }
}


// ------------------------------------ Métodos View ------------------------------
void inicializaWebServer(){
  webServer.on("/restart_equipamento.html", HTTP_GET, templateRestartEquipamento);
  webServer.on("/restart_equipamento.html", HTTP_POST, restartDevice);
  webServer.on ("/restaurar_configuracao.html", HTTP_GET, templateRestaurarConfiguracaoFabrica);  
  webServer.on("/restaurar_configuracao.html", HTTP_POST, restaurarConfiguracaoFabrica);
  webServer.on ( "/sensores.html", templateSensores );
  webServer.on ( "/iluminacao.html", templateIluminacao);
  webServer.on ( "/dashboard.html", templateDashboard );
  webServer.on ( "/", handleRoot );
  webServer.onNotFound(handleNotFound);  
  webServer.begin();
  Serial.println ( "HTTP server started" );
}
void handleRoot(){
  templateDashboard();
}
void handleNotFound(){
  webServer.send (404, "text/html", getTemplatePagina("Página não encontrada","Erro 404") );
}
 
// Método para gerar a estrutura do template padrão
String getTemplatePagina(String conteudoHtml, String tituloPagina){
  String templatePagina = "";
  if (tituloPagina == "") tituloPagina = "ApoenaIOT";  
  templatePagina += "<!DOCTYPE html><html lang='pt-br'><head><meta http-equiv='X-UA-Compatible' content='IE=edge'><meta name='viewport' content='width=device-width, initial-scale=1'><meta charset='utf-8'><meta http-equiv='X-UA-Compatible' content='IE=edge'><meta name='description' content='ApoenaIOT'><meta name='author' content='Lucas Apoena'><link href='https://rawgit.com/lucasapoena/apoenaIOT-slave/master/template/css/css.css' rel='stylesheet'><title>ApoenaIOT - Central Slave "+ versaoApp +"</title></head><body><div id='wrapper'><nav class='navbar navbar-default navbar-static-top' style='margin-bottom: 0'><div class='navbar-header'><button type='button' class='navbar-toggle' data-toggle='collapse' data-target='.navbar-collapse'><span class='sr-only'>Menu</span><span class='icon-bar'></span><span class='icon-bar'></span><span class='icon-bar'></span></button><a class='navbar-brand' href='dashboard.html'>ApoenaIOT - Central Slave</a></div><ul class='nav navbar-top-links navbar-right'><li><a href='#' accesskey='1'><i title='Ajustar Fonte' aria-hidden='true' class='fa fa-font fa-fw'></i></a></li><li><a href='#' accesskey='2'><i title='Ajustar Contraste' aria-hidden='true' class='fa fa-low-vision fa-fw'></i></a></li><li class='dropdown'><a aria-label= 'Informações gerais de desempenho da placa' class='dropdown-toggle' data-toggle='dropdown' href='#'><i title='Informações de desempenho' aria-hidden='true' class='fa fa-tasks fa-fw'></i><i aria-hidden='true' aria-hidden='true' class='fa fa-caret-down'></i></a><ul class='dropdown-menu dropdown-messages'><li><a href='#'><div><p><strong>CPU</strong><span class='pull-right text-muted'>40% Utilizado</span></p><div class='progress progress-striped active'><div title='Utilizando 40% da CPU' class='progress-bar progress-bar-success' role='progressbar' aria-valuenow='40' aria-valuemin='0' aria-valuemax='100' style='width: 40%'><span class='sr-only'></span></div></div></div></a></li><li class='divider'></li><li><a href='#'><div><p><strong>Memoria</strong><span class='pull-right text-muted'>80% Utilizado</span></p><div class='progress progress-striped active'><div title='Utilizando 80% da Memória' class='progress-bar progress-bar-danger' role='progressbar' aria-valuenow='80' aria-valuemin='0' aria-valuemax='100' style='width: 80%'><span class='sr-only'></span></div></div></div></a></li><li class='divider'></li><li><a href='#'><div><p><strong>Armazenamento</strong><span class='pull-right text-muted'>20% Utilizado</span></p><div class='progress progress-striped active'><div title='Utilizando 20% do Armazenamento' class='progress-bar progress-bar-info' role='progressbar' aria-valuenow='20' aria-valuemin='0' aria-valuemax='100' style='width: 20%'><span class='sr-only'></span></div></div></div></a></li></ul></li></ul><div class='navbar-default sidebar' role='navigation'><div class='sidebar-nav navbar-collapse'><ul class='nav' id='side-menu'><li><a href='dashboard.html' accesskey='3'><i aria-hidden='true' class='fa fa-dashboard fa-fw'></i> Dashboard</a></li><li><a href='#'><i aria-hidden='true' class='fa fa-sitemap fa-fw'></i> Dispositivos<span class='fa arrow'></span></a><ul class='nav nav-second-level'><li><a href='iluminacao.html' accesskey='4'><i aria-hidden='true' class='fa fa-lightbulb-o' aria-hidden='true'></i> Iluminação</a></li><li><a href='sensores.html' accesskey='5'><i aria-hidden='true' class='fa fa-bar-chart-o fa-fw'></i> Sensores</a></li></ul></li><li><a href='#'><i aria-hidden='true' class='fa fa-wrench fa-fw'></i> Configurações<span class='fa arrow'></span></a><ul class='nav nav-second-level'><li><a href='restaurar_configuracao.html' accesskey='6'>Restaurar Configurações</a></li><li><a href='restart_equipamento.html' accesskey='7'>Reboot</a></li></ul></li></ul></div></div></nav><div id='page-wrapper'><div class='container-fluid'><div class='row'><div class='col-lg-12'><h1 class='page-header'>"+tituloPagina+"</h1></div></div><div class='row'> "+conteudoHtml+"</div></div></div></div><script src='https://cdn.rawgit.com/lucasapoena/apoenaIOT-slave/master/template/js/js.js'></script></body></html>";
  return templatePagina;
}

void templateDashboard(){
  String conteudoHtml = "";
  int dispositivosLigados = 0;
  int dispositivosDesligados = 0;  
  for (uint8_t i=0;i<QTD_RELES;i++){ 
    dispositivosLigados += int(statusGpioReles[i]);    
  }
  dispositivosDesligados = int(QTD_RELES) - dispositivosLigados;
  conteudoHtml += "<div class='row'><div class='col-lg-3 col-md-6'><div class='panel panel-green'><div class='panel-heading'><div class='row'><div class='col-xs-3'><i class='fa fa-bolt fa-5x'></i></div><div class='col-xs-9 text-right'><div class='huge'>"+ String(dispositivosLigados) +"</div><div>Ligados</div></div></div></div><a href='iluminacao.html'><div class='panel-footer'><span class='pull-left'>Visualizar...</span><span class='pull-right'><i class='fa fa-arrow-circle-right'></i></span><div class='clearfix'></div></div></a></div></div><div class='col-lg-3 col-md-6'><div class='panel panel-red'><div class='panel-heading'><div class='row'><div class='col-xs-3'><i class='fa fa-power-off fa-5x'></i></div><div class='col-xs-9 text-right'><div class='huge'>"+ String(dispositivosDesligados) +"</div><div>Desligados</div></div></div></div><a href='iluminacao.html'><div class='panel-footer'><span class='pull-left'>Visualizar...</span><span class='pull-right'><i class='fa fa-arrow-circle-right'></i></span><div class='clearfix'></div></div></a></div></div></div><div class='row'><div class='col-lg-12'><div class='panel panel-default'><div class='panel-heading'> Informações da Placa</div><div class='panel-body'><div class='table-responsive'><table class='table table-striped table-bordered table-hover'><thead><tr><th>#</th><th>Descrição</th></tr></thead><tbody><tr><td>Sistema</td><td>ApoenaIOT-Slave "+ versaoApp +"</td></tr><tr><td>Modelo da placa</td><td>NodeMCU v3 Lolin</td></tr><tr><td>Serial</td><td>15as15dd00das5a4</td></tr><tr><td>Hostname</td><td>"+WiFi.hostname()+"</td></tr><tr><td>SSID</td><td>"+WiFi.SSID()+"</td></tr><tr><td>IP</td><td>"+WiFi.localIP().toString()+"</td></tr><tr><td>Subnet</td><td>"+WiFi.subnetMask().toString()+"</td></tr><tr><td>Gateway</td><td>"+WiFi.gatewayIP().toString()+"</td></tr></tbody></table></div></div></div></div></div>";
  webServer.send ( 200, "text/html", getTemplatePagina(conteudoHtml,"Dashboard") );
}

void templateIluminacao(){
  String nomesDispositivos[QTD_RELES] = {"Luz Principal","Abajur - Star Wars","Fita de Led"};
  String conteudoHtml = "";
  conteudoHtml += "<div class='row'>";
  for (uint8_t i=0; i<QTD_RELES; i++){
    if (webServer.hasArg("button_"+String(i))){
      atualizaStatusRele(uint8_t(i), webServer.arg("button_"+String(i)));
    }
    conteudoHtml += "<div class='col-lg-3'>";
    if(statusGpioReles[i]){ 
      // Caso Ligado
      conteudoHtml += "<div class='panel panel-green'><div class='panel-heading'><i class='fa fa-bolt'></i> (Ligado) Lâmpada - Cod.: D"+String(i)+"</div><div class='panel-body'><form action='iluminacao.html' method='POST'><button type='submit' name='button_"+String(i)+"' value='0' class='btn btn-danger btn-lg' ><i class='fa fa-power-off'></i><br/>Desligar</button></form></div><div class='panel-footer'>"+nomesDispositivos[i]+"</div></div>";      
    }else{
      // Caso desligado
      conteudoHtml += "<div class='panel panel-red'><div class='panel-heading'><i class='fa fa-power-off'></i> (Desligado) Lâmpada - Cod.: D"+String(i)+"</div><div class='panel-body'><form action='iluminacao.html' method='POST'><button type='submit' name='button_"+String(i)+"' value='1' class='btn btn-success btn-lg'><i class='fa fa-bolt'></i><br/>Ligar</button></form></div><div class='panel-footer'>"+nomesDispositivos[i]+"</div></div>";
   }    
    conteudoHtml += "</div>";
  }
  conteudoHtml += "</div>";
  
  webServer.send ( 200, "text/html", getTemplatePagina(conteudoHtml,"Iluminação") );
}

void templateSensores(){
  String conteudoHtml = "";
  conteudoHtml +="<div class='row'><div class='col-lg-12'><div class='panel panel-default'><div class='panel-heading'> Sensores</div><div class='panel-body'><div class='table-responsive'><table class='table table-striped table-bordered table-hover'><thead><tr><th>#</th><th>Descrição</th><th>Medição</th></tr></thead><tbody><tr><td>Sensor</td><td>Temperatura</td><td>35º</td></tr><tr><td>Sensor</td><td>Umidade</td><td>20%</td></tr></tbody></table></div></div></div></div></div>";
   webServer.send ( 200, "text/html", getTemplatePagina(conteudoHtml,"Sensores") );
}

void templateRestaurarConfiguracaoFabrica(){
  String conteudoHtml = "";
  conteudoHtml +="<div class='col-lg-12'><div class='panel panel-default'><div class='panel-heading'> Restaurar Configurações de Fábrica</div><div class='panel-body'><p>Clique no botão abaixo para realizar a restauração de fábrica da sua Apoena - Cetral Slave</p><form action='restaurar_configuracao.html' method='POST'><button type='button submit' onclick='confirm();' class='btn btn-default'>Restore</button></form></div><div class='panel-footer'></div></div></div>";
  webServer.send ( 200, "text/html", getTemplatePagina(conteudoHtml,"Restaurar") );
}

void templateRestartEquipamento(){
  String conteudoHtml = "";
  conteudoHtml +="<div class='col-lg-12'><div class='panel panel-default'><div class='panel-heading'> Reboot - Equipamento</div><div class='panel-body'><p>Clique no botão abaixo para realizar o reboot da sua Apoena - Cetral Slave</p><form action='restart_equipamento.html' method='POST'><button type='button submit' onclick='confirm();' class='btn btn-default'>Reboot</button></form></div><div class='panel-footer'></div></div></div>";
  webServer.send ( 200, "text/html", getTemplatePagina(conteudoHtml,"Restaurar") );
}

