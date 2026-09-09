Diagrama de flujo del sistema de ultafiltracion para potabilizar agua

1) TANQUE DE AGUA SUCIA: Elevado para bajar por gravedad  
2) SEDIMENTADOR: Relación agua clarificante  
* Llave cerrar/abrir \> ingreso de agua sucia al sedimentador (sensor)  
* Paleta sedimentador (girar) velocidad rápida 1 min velocidad lenta 5 min \- configurar con el esp32 \-velocidad de giro independiente de la cantidad de agua  
* Sensor del nivel del tanque (arriba)  
* Sensor del nivel de sedimentador (abajo)  
* Llave abrir/cerrar \> salida de agua clarificada   
3) BOMBA:   
* Ingresa agua clarificada   
* Sale agua al filtro   
4) FILTRO  
* Sensor de conductividad (antes)  
* Sensor de presion (antes) ingreso al filtro  
* Sensor de caudal (antes) ingreso al filtro  
* Sensor de presion (retorno/retrolavado) hacia el tanque sedimentador  
* Sensor de conductividad (salida permeado)  
* Sensor de presion (salida permeado)  
* Sensor de caudal (salida permeado)  
5) TANQUE PERMEADO