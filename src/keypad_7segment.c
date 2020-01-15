/* Copyright 2016, Eric Pernia.
 * All rights reserved.
 *
 * This file is part sAPI library for microcontrollers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Date: 2016-07-28
 */

/*==================[inclusions]=============================================*/

//#include "keypad_7segment.h"   // <= own header (optional)
#include "sapi.h"  // <= sAPI header
typedef enum{principal,modificar_riego,modificar_horario,confirmar_cambios,modificar_humedad,inicio,mostrar_estado,mostrar_horario,supervisar}estado;  //utilizado para controlar la mef principal (faltan agregar estados)

#define MANIANA 0
#define NOCHE 1

const char tickChar[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00001,
  0b00010,
  0b10100,
  0b01000
};

const char upChar[] = {
  0b00000,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};

const char downChar[] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b00000
};

const char cancelChar[] = {
  0b00000,
  0b10001,
  0b01010,
  0b00100,
  0b01010,
  0b10001,
  0b00000,
  0b00000
};

const char dayChar[] = {
  0b00000,
  0b10101,
  0b01110,
  0b11111,
  0b01110,
  0b10101,
  0b00000,
  0b00000
};

const char nightChar[] = {
  0b00111,
  0b01100,
  0b11000,
  0b11000,
  0b11000,
  0b11000,
  0b01100,
  0b00111
};

// Teclado
keypad_t keypad;
// Variable para guardar la tecla leida
uint16_t tecla = 0;
uint16_t aux = 66;
// variables para humedad
uint16_t hum=40;
uint16_t hum_aux=40;
//buffer para conversion de nros
static char final[10];
//variables para mef
bool_t continuar; //variable para validar tecla seleccionada en estados modificar
estado e; // para la mef
//variables para el horario
uint16_t h=MANIANA; //horario seleccionado por defecto
uint16_t aux_h; // var auxiliar de horario
//variables para riego
uint16_t r = 1; // 1-> vez por dia, 2 2 veces por semana, 3 -> 1 vez por semana
uint16_t aux_r;
uint16_t aux_sem =1; //semana 1 por defecto
uint16_t contdia=0; // por defecto en 0

//variables flag para el modo supervisar
uint16_t flag=0; //flag
uint16_t flag_cont =0; // flag que avisa si todas las condiciones se cumplen o no

/* Variables de delays no bloqueantes */
delay_t delay1;
delay_t delay2;
delay_t delay3;

/* Variable para almacenar el valor leido del ADC CH1 */
uint16_t muestra = 0;
/*mapeo*/
uint32_t nuevoval;

//RTC
/* Estructura RTC */
rtc_t rtc;
bool_t val = 0;

//arreglo RTC
rtc_t arreglo[2];

/**Contadores**/

/* FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE RESET. */
int main(void) {

	/* ------------- INICIALIZACIONES ------------- */

	/* Inicializar la placa */
	boardConfig();

	/* Configuracion de pines para el Teclado Matricial*/



	// Filas --> Salidas
	uint8_t keypadRowPins1[4] = { RS232_TXD, // Row 0
			CAN_RD,    // Row 1
			CAN_TD,    // Row 2
			T_COL1     // Row 3
			};

	// Columnas --> Entradas con pull-up (MODO = GPIO_INPUT_PULLUP)
	uint8_t keypadColPins1[4] = { T_FIL0,    // Column 0
			T_FIL3,    // Column 1
			T_FIL2,    // Column 2
			T_COL0     // Column 3
			};

	keypadConfig(&keypad, keypadRowPins1, 4, keypadColPins1, 4);

	// Inicializar LCD de 16x2 (caracteres x lineas) con cada caracter de 5x2 pixeles
	lcdInit(16, 2, 5, 8);

	/* Inicializar AnalogIO */
	/* Posibles configuraciones:
	 *    ADC_ENABLE,  ADC_DISABLE,
	 */
	adcConfig(ADC_ENABLE); /* ADC */



	/* Inicializar Retardo no bloqueante con tiempo en ms */
	delayConfig(&delay1, 500);
	delayConfig(&delay2, 200);
	delayConfig(&delay3, 1000);

	/* ConfiguraciÃ³n de estado inicial del Led */
	bool_t ledState1 = OFF;

	//RTC configuracion asignaciones y demas
	rtc.year = 2019;
	rtc.month = 1;
	rtc.mday = 10;
	rtc.wday = 0;
	rtc.hour = 12;
	rtc.min = 0;
	rtc.sec = 0;

	//inicializar arreglo del rtc
	arreglo[0].hour=8; //MAÑANA
	arreglo[0].min=0;
	arreglo[0].sec=0;
	arreglo[0].year = 2019;
	arreglo[0].month = 1;
	arreglo[0].mday = 10;
	arreglo[0].wday = 0;

	arreglo[1].hour=19; //NOCHE
	arreglo[1].min=0;
	arreglo[1].sec=0;
	arreglo[1].year = 2019;
	arreglo[1].month = 1;
	arreglo[1].mday = 10;
	arreglo[1].wday = 0;
	/* Inicializar RTC */
	val = rtcConfig(&rtc); //HAY QUE INICIALIZAR RTC ARREGLO PORQUE MARCA ERRO
	delay(2000);

	gpioConfig( RS232_RXD, GPIO_OUTPUT );

	   // Cargar el caracter a CGRAM
	   // El primer parÃ¡metro es el cÃ³digo del caracter (0 a 7).
	   // El segundo es el puntero donde se guarda el bitmap (el array declarado
	   // anteriormente)
	   lcdCreateChar( 0, cancelChar );
	   lcdCreateChar( 1, tickChar );
	   lcdCreateChar( 2, upChar );
	   lcdCreateChar( 3, downChar);
	   lcdCreateChar(4, dayChar);
	   lcdCreateChar(5, nightChar);
		continuar=FALSE; // seteo en Falso la variable continuar


	/* ------------- REPETIR POR SIEMPRE ------------- */
	while (1) {

	}

	/* NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa no es llamado
	 por ningun S.O. */
	return 0;
}


void modificarHorario(){
	lcdGoToXY(0, 0); // Poner cursor en 0, 0
	lcdSendStringRaw("Eliga horario: ");
	//opciones a elegir horario riego
	mostrarOpciones(modificar_horario);
	while(!keypadRead(&keypad, &tecla));
	if ((tecla == 3 || tecla ==7  || tecla == 15)){ // Si selecciono una tecla VALIDA continua
		continuar = TRUE;
	}
	else{    // CASO CONTRARIO, sigue mostrando las opciones
		continuar = FALSE;
		if (( keypadRead(&keypad, &tecla)) && (tecla != 3 || tecla != 7  || tecla != 15)){ // SI SELECCIONO UNA TECLA INVALIDA
			limpiarLineaDisplay(1);
			lcdSendStringRaw("TECLA INVALIDA!");
			//delay (500);
		}
	}
	// SELECCIONO UNA TECLA VALIDA, modifico variables auxiliares
	if (continuar == TRUE){
		limpiarLineaDisplay(1);
		switch (tecla){
		case 3: // HORARIO NOCHE
			lcdSendStringRaw("NOCHE");
			aux_h=NOCHE;

			break;
		case 7:  // HORARIO MAÃ‘ANA
			lcdSendStringRaw("MAÑANA");
			aux_h=MANIANA;

			break;
		case 15: // opcion cancelar
			lcdSendStringRaw("CANCELAR");
			e=principal;
			break;
		default:  // nunca deberia llegar al caso default
			//cant_riego=1;
			//e=principal;
			break;
		}
	}
	delay(1000);

}
void confirmarCambios (){
	limpiarLineaDisplay(0); //limpia y posiciona
		//opciones a elegir de confirmar cambios
		mostrarOpciones(confirmar_cambios);
		while(!keypadRead(&keypad, &tecla));
		if ((tecla == 3 || tecla ==7 )){ // Si selecciono una tecla VALIDA continua
			continuar = TRUE;
		}
		else{    // CASO CONTRARIO, sigue mostrando las opciones
			continuar = FALSE;
			if (( keypadRead(&keypad, &tecla)) && (tecla != 3 || tecla != 7)){ // SI SELECCIONO UNA TECLA INVALIDA
				limpiarLineaDisplay(1);
				lcdSendStringRaw("TECLA INVALIDA!");
			}
		}
		// SELECCIONO UNA TECLA VALIDA, modifico variables auxiliares
		if (continuar == TRUE){
			limpiarLineaDisplay(1);
			switch (tecla){
			case 3: // CASO  A  cambios guardados
				lcdSendStringRaw("CAMBIO OK ");
				lcdData(1);

				h = aux_h;
				r = aux_r;
				hum = hum_aux;

				break;

			case 7:  // CASO B cancelar cambios
				lcdSendStringRaw("CANCELADO ");
				lcdData(0);
				break;

			default:  // nunca deberia llegar al caso default

				break;
			}
		}
		delay(1000);
		/*retorna a principal*/
		e=principal;
}

/* Enviar fecha y hora en formato "DD/MM/YYYY, HH:MM:SS" */
void imprimirHora(rtc_t * rtc) {

	/* Conversion de entero a ascii con base decimal */
	itoa((int) (rtc->hour), (char*) final, 10); /* 10 significa decimal */
	/* Envio la hora */
	if ((rtc->hour) < 10)
		lcdSendStringRaw("0");
	lcdSendStringRaw(final);
	lcdSendStringRaw(":");

	/* Conversion de entero a ascii con base decimal */
	itoa((int) (rtc->min), (char*) final, 10); /* 10 significa decimal */
	/* Envio los minutos */
	// uartBuff[2] = 0;    /* NULL */
	if ((rtc->min) < 10)
		lcdSendStringRaw("0");
	lcdSendStringRaw(final);
	lcdSendStringRaw(":");

	/* Conversion de entero a ascii con base decimal */
	itoa((int) (rtc->sec), (char*) final, 10); /* 10 significa decimal */
	/* Envio los segundos */
	if ((rtc->sec) < 10)
		lcdSendStringRaw("0");
	lcdSendStringRaw(final);

}


void mostrar_supervisar(){
	/* Leer fecha y hora */
	val = rtcRead(&rtc);
	if((rtc.hour == arreglo[h].hour)&&(rtc.min == arreglo[h].min)&&(rtc.sec == arreglo[h].sec)){
		flag=1; // activo el flag
		contdia++; //incrementa el dia porque se cumplio el horario establecido por el usuario
	}
	if (flag == 1){
		//Caso en el que la hora programada coincida evaluo la opcion de riego que eligio el usuario
		switch (r) {
			case 1: //1 vez por dia riega
				flag_cont=1;
				break;
			case 2: // 2 veces a la semana
				if ((contdia==1)||(contdia==4)){
					aux_sem++; // incrementa la cant de veces a la semana
					flag_cont=1;
				}
				if(aux_sem==3){ //si ya pasaron las 2 veces a la semana que tenia que regar, reset
					aux_sem=1;
					contdia=0;
				}
				break;
			case 3: //1 vez a la semana
				if (contdia==1) flag_cont=1;
				if (contdia==7){
					contdia=0;
				}
				break;
			default:
				break;
		}
		/*Si se cumplen las condiciones de riego entonces evaluo*/
		/* delayRead retorna TRUE cuando se cumple el tiempo de retardo */
		if (delayRead(&delay1)&& flag_cont ==1 ) {
			/*Interfaz del display*/
			limpiarLineaDisplay(0);
			lcdGoToXY(0,0);
			lcdSendStringRaw("Regando..");
			/* Leo la Entrada Analogica AI0 - ADC0 CH1 */
			muestra = adcRead(CH1);

			/*TRUNCANDO VALORES*/
			if (muestra < 485) {
				muestra = 485;
			} else if (muestra > 1023) {
				muestra = 1023;
			}
			/*FIN DEL TRUNCADO DE VALORES*/

			nuevoval = map(muestra, 485, 1024, 100, 0);
			/* Conversion de muestra entera a ascii con base decimal */

			/*BITS EN ESCALA DECIMAL*/
			itoa(muestra, final, 10); /* 10 significa decimal */

			/*BITS EN ESCALA DE 0 A 100*/
			itoa(nuevoval, final, 10); /* 10 significa decimal */

			/*CONTROL DE RIEGO CUANDO ES LA HORA*/
			if (nuevoval < 80) {
				gpioWrite(RS232_RXD, 1);

			} else {
				gpioWrite(RS232_RXD, 0);
				flag=0; //reseteo flag
				flag_cont=0;
				/*Interfaz del display */
				limpiarLineaDisplay(0);
				lcdGoToXY(0,0);
				lcdSendStringRaw("Riego finalizado");
				delay(2000);
			}

		}
	}
	else{
		/*Caso en el que no cumpla la hora asignada, no se enciende el motor y solo muestra la hora actual y que el
		 * sistema se encuentra activado */
		//Estando en este estado muestro el reloj en primer linea y en segunda linea muestro el sistema activado
		limpiarLineaDisplay(0);
		lcdGoToXY(0,2);
		lcdSendStringRaw("SISTEMA ACTIVADO");
		lcdGoToXY(0,0);
		lcdSendStringRaw("Hora ");
		if (delayRead(&delay3)){ // Cada un segundo actualiza
			/* Leer fecha y hora */
			val = rtcRead(&rtc);
			//imprime hora en lcd
			imprimirHora(&rtc);
		}
	}

}

void mostrar_principal (){
	limpiarLineaDisplay(0); //limpia y posiciona
		lcdSendStringRaw("Que desea hacer?");
		//opciones a elegir de estado principal
		mostrarOpciones(principal);
		while(!keypadRead(&keypad, &tecla));
		if ((tecla == 3 || tecla ==7 )){ // Si selecciono una tecla VALIDA continua
			continuar = TRUE;
		}
		else{    // CASO CONTRARIO, sigue mostrando las opciones
			continuar = FALSE;
			if (( keypadRead(&keypad, &tecla)) && (tecla != 3 || tecla != 7)){ // SI SELECCIONO UNA TECLA INVALIDA
				limpiarLineaDisplay(1);
				lcdSendStringRaw("TECLA INVALIDA!");
			}
		}
		// SELECCIONO UNA TECLA VALIDA, modifico variables auxiliares
		if (continuar == TRUE){
			limpiarLineaDisplay(1);
			switch (tecla){
			case 3: // CASO  A encender alarma
				lcdSendStringRaw("ALARMA ON ");
				lcdData(6);
				//aux_h=NOCHE;
				//e= confirmar_cambios;
				break;
			case 7:  // CASO B Configurar alarma
				lcdSendStringRaw("CONFIG ALARMA ");
				lcdData(7);
				//aux_h=NOCHE;
				//e= confirmar_cambios;
				break;
			default:  // nunca deberia llegar al caso default
				//cant_riego=1;
				//e=principal;
				break;
			}
		}
		delay(1000);

}
/*funcion para hacer una copia de seguridad
 * parametros : NINGUNO
 * retorno: NADA
 *
 * */
void set_aux_backup() {
	aux_r = r;
	aux_h = h;
	hum_aux = hum;
}

void modificarRiego(){
		set_aux_backup();//hago una copia de seguridad de los auxiliares con los valores pordefecto en caso de cancelar
		lcdGoToXY(0, 0); // Poner cursor en 0, 0
		lcdSendStringRaw("Eliga horario: ");
		//opciones a elegir horario riego
		mostrarOpciones(modificar_riego);
		while(!keypadRead(&keypad, &tecla));
		if ((tecla == 3 || tecla ==7  || tecla == 11|| tecla == 15)){ // Si selecciono una tecla VALIDA continua
			continuar = TRUE;
		}
		else{    // CASO CONTRARIO, sigue mostrando las opciones
			continuar = FALSE;
			if (( keypadRead(&keypad, &tecla)) && (tecla != 3 || tecla != 7 || tecla != 11 || tecla != 15)){ // SI SELECCIONO UNA TECLA INVALIDA
				limpiarLineaDisplay(1);
				lcdSendStringRaw("TECLA INVALIDA!");
				//delay (500);
			}
		}
		// SELECCIONO UNA TECLA VALIDA, modifico variables auxiliares
		if (continuar == TRUE){
			limpiarLineaDisplay(1);
			switch (tecla){
			case 3: // riego 1 vez por dia
				lcdSendStringRaw("1 dia");
				aux_r=1;
				alternar();
				break;
			case 7:  // riego 2 veces por semana
				lcdSendStringRaw("2 semana");
				aux_r=2;
				alternar();
				break;
			case 11:  // riego 1 vez por semana
				lcdSendStringRaw("1 semana");
				aux_r=3;
				alternar();
				break;
			case 15: // opcion cancelar
				lcdSendStringRaw("CANCELADO");
				e=principal;
				break;
			default:  // nunca deberia llegar al caso default

				break;
			}
		}
		delay(2000);

}
/***
 * funcion: le muestra al usuario en pantalla las opciones a eleccion para alternar estando anteriormente
 * en el estado modificar riego
  ***/
void alternar(estado e) {
	lcdClear();
	lcdGoToXY(0, 0);
	lcdSendStringRaw("Elija opcion:");
	delay(1500);
	if (e == modificar_riego) {
		lcdClear();
		lcdGoToXY(0, 0);
		lcdSendStringRaw("A: Mod Hor C: ");
		lcdData(1);
		lcdGoToXY(0, 2);
		lcdSendStringRaw("B: Mod Hum ");
	}

	if (e == modificar_humedad) {
		lcdClear();
		lcdGoToXY(0, 0);
		lcdSendStringRaw("A: Mod Hor B: ");
		lcdData(1);
	}
	if (e == modificar_horario) {
		lcdClear();
		lcdGoToXY(0, 0);
		lcdSendStringRaw("A: Mod Hum B: ");
		lcdData(1);
	}

	while(!keypadRead(&keypad, &tecla));

	if ((tecla == 3 || tecla ==7  || tecla == 11)){ // Si selecciono una tecla VALIDA continua
		switch (e) {
			case modificar_riego:
				 continuar = TRUE;
				break;
			case modificar_horario:
				if(tecla != 11) continuar = TRUE;
				else continuar = FALSE;
				break;
			case modificar_humedad:
				if(tecla != 11) continuar = TRUE;
				else continuar = FALSE;
				break;
			default:
				//NO deberia entrar aqui :(
				break;
		}
	}
	else{    // CASO CONTRARIO, sigue mostrando las opciones
		continuar = FALSE;
		if (( keypadRead(&keypad, &tecla)) && (tecla != 3 || tecla != 7 || tecla != 11)){ // SI SELECCIONO UNA TECLA INVALIDA
			limpiarLineaDisplay(1);
			lcdSendStringRaw("TECLA INVALIDA!");
			//delay (500);
		}
	}

	// SELECCIONO UNA TECLA VALIDA, modifico variables auxiliares
	if (continuar == TRUE){
		limpiarLineaDisplay(1);
		switch (tecla){
		case 3: // riego 1 vez por dia
			lcdSendStringRaw("Eligio Horario");
			e=modificar_horario;
			break;
		case 7:  // riego 2 veces por semana
			lcdSendStringRaw("Eligio Humedad");
			e=modificar_humedad;
			break;
		case 11:  // riego 1 vez por semana
			lcdSendStringRaw("Confirmar cambios");
			e=confirmar_cambios;
			break;
		default:  // nunca deberia llegar al caso default

			break;
		}
	}
	delay(2000);
}
void limpiarLineaDisplay(uint16_t l){ // cambiar de nombre!
	if (l == 0){
		lcdGoToXY(0, 0); // Poner cursor en 0, 0
		lcdClear(); // Borrar la pantalla
	}
	if (l == 1){
		lcdGoToXY(0, 2); // Poner cursor en 1, 0
		lcdClear(); // Borrar la pantalla
		lcdGoToXY(0,2);
	}
}

/**************************************************************************************************************
        Funcion mostrarOpciones
        parametros: estado e
        descripcion: muestra diversas opciones en el display de acuerdo al parametro seleccionado
        return null
 ****************************************************************************************************************/

void modificarHumedad() {

	lcdClear(); // Borrar la pantalla
	lcdGoToXY(0, 0); // Poner cursor en 0, 0

	lcdSendStringRaw("Set H: ");

	itoa(hum_aux, final, 10);

	lcdSendStringRaw(final);
	lcdGoToXY(0, 2);

	lcdSendStringRaw("A:");
	lcdData(2);

	lcdSendStringRaw(" ");
	lcdSendStringRaw("B:");
	lcdData(3);

	lcdSendStringRaw(" ");
	lcdSendStringRaw("C:");
	lcdData(1);

	delay(800);
	lcdGoToXY(0, 2); // Poner cursor en 0, 0
	lcdSendStringRaw("               ");

	lcdGoToXY(0, 2); // Poner cursor en 0, 0
	lcdSendStringRaw("D:");
	lcdData(0);

	if (keypadRead(&keypad, &tecla)) {
		switch (tecla) {
		case 3: //A
			hum_aux = hum_aux + 1;
			if (hum_aux == 101) {
				hum_aux = 0;
			}
			break;
		case 7: //B
			hum_aux = hum_aux - 1;
			if (hum_aux == 65535) {
				hum_aux = 100;
			}
			break;
		case 11: //C
			hum = hum_aux;
			break;
		case 15: //D
			lcdGoToXY(0, 2);
			lcdSendStringRaw("OP CANCELADA");
			break;
		default:
			lcdSendStringRaw("ERROR");
			break;
		}
	}
	delay(200);

}




/**************************************************************************************************************
        Funcion mostrarOpciones
        parametros: estado e
        descripcion: muestra diversas opciones en el display de acuerdo al parametro seleccionado
        return null
 ****************************************************************************************************************/
void mostrarOpciones(estado e){
	switch(e){
	case modificar_riego:
		limpiarLineaDisplay(1);
		lcdSendStringRaw("A: 1 vez por dia");
		delay (1000);
		limpiarLineaDisplay(1);
		lcdSendStringRaw("B: 3 por semana ");
		delay (1000);
		limpiarLineaDisplay(1);
		lcdSendStringRaw("C: 1 por semana ");
		delay (1000);
		limpiarLineaDisplay(1);
		lcdSendStringRaw("D: CANCELAR ");
		delay (1000);
		limpiarLineaDisplay(0);
		lcdSendStringRaw("Elija una opcion");
		delay(1000);
		limpiarLineaDisplay(0);
		lcdSendStringRaw("A:1D B:3S C:1S ");
		lcdGoToXY(0,2);
		lcdSendStringRaw("D:");
		lcdData(0);
		break;
	case modificar_horario:
		limpiarLineaDisplay(1);
		lcdGoToXY(0,2);
		lcdSendStringRaw("A: NOCHE");
		delay (1000);
		limpiarLineaDisplay(1);
		lcdGoToXY(0,2);
		lcdSendStringRaw("B: MAÑANNA ");
		delay (1000);
		limpiarLineaDisplay(1);
		lcdGoToXY(0,2);
		lcdSendStringRaw("D: CANCELAR ");
		delay (1000);
		limpiarLineaDisplay(0);
		lcdSendStringRaw("Elija una opcion");
		lcdGoToXY(0,2);
		lcdSendStringRaw("A:");
		lcdData(5);
		lcdSendStringRaw(" B:");
		lcdData(4);
		lcdSendStringRaw(" D:");
		lcdData(0);
		break;
	case confirmar_cambios:
		limpiarLineaDisplay(1);
		lcdSendStringRaw("A: SI");
		delay (200);

		limpiarLineaDisplay(1);
		lcdSendStringRaw("B: NO");
		delay (200);

		break;

	default:
		break;
	}
}
/********************************************************************************************************************
        Funcion  mostrarHorario
        parametros: ninguno
        descripcion: imprime en el display lcd el horario que se eligio para regar la planta
        return null
 ********************************************************************************************************************/
void mostrarHorario(){
	limpiarLineaDisplay(0);
	lcdGoToXY(0, 0); // Poner cursor en 0, 0
	lcdSendStringRaw("Hora de riego: ");
	lcdGoToXY(0, 2); // Poner cursor en 1, 0
	if (h == NOCHE){
		lcdSendStringRaw("NOCHE");
	}
	else {
		lcdSendStringRaw("MAÑANA");
	}
	delay (2000);
	e=supervisar;
}
/********************************************************************************************************************
        Funcion  mostrarRiego
        parametros: ninguno
        descripcion: imprime en el display lcd la cantidad de veces que se rego la planta
        return null
 ********************************************************************************************************************/
void mostrarRiego(){
	limpiarLineaDisplay(0);
	lcdGoToXY(0, 0); // Poner cursor en 0, 0
	if (r == 2){
		lcdSendStringRaw("Riego: 2 por sem");
	}
	if (r == 3){
		lcdSendStringRaw("Riego: 1 por sem ");
	}
	if (r == 1){
		lcdSendStringRaw("Riego: 1 por dia");
	}
	delay (200);
	e=supervisar;
}
void mef_update(estado e){

	switch(e){

	case modificar_riego:
		modificarRiego();
		break;
	case modificar_horario:
		modificarHorario();
		break;
	case confirmar_cambios:
		confirmarCambios();
		break;
	case supervisar:
		mostrar_supervisar();
		break;
	case mostrar_horario: //
		mostrarHorario();
		break;
	case mostrar_estado: //
		mostrarRiego();
		break;
	case principal:
		mostrar_principal();
		break;
	case inicio:
		Inicio();
		break;
	default:
		break;
	}
}
/*==================[end of file]============================================*/
