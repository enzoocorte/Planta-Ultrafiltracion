#include <cstdio>
#include <cmath>
unsigned long g_ledc_freq = 0, g_ledc_duty = 0;
#include "config.h"
#include "Bomba.h"
int main(){
  printf("Simulacion de Bomba::tick() (dt=50 ms, el lazo real del firmware)\n");
  printf(" ACEL_ARRANQUE=%.1f  ACEL_NOMINAL=%.1f  umbral de aproximacion delta<3.0 RPM\n\n",
         (double)ACEL_ARRANQUE_RPM_S,(double)ACEL_NOMINAL_RPM_S);
  printf("  Consigna   t hasta enRegimenEstable()   volumen bombeado durante la rampa\n");
  const float objetivos[]={15,20,25,30,40,50,60,70,80,90,100};
  for(float obj: objetivos){
    Bomba b; b.begin(); b.setRPM(obj); b.arrancar();
    float t=0, vol=0; const float dt=0.050f;
    while(!b.enRegimenEstable() && t<300.f){ b.tick(dt); vol += b.rpmActual()*ML_POR_VUELTA*dt/60.0f; t+=dt; }
    printf("   %5.0f RPM        %6.2f s                    %7.1f mL  (=%.2f s de regimen perdido en 60 s)\n",
           obj,t,vol, t);
  }
  // frenado
  Bomba b; b.begin(); b.setRPM(100); b.arrancar();
  for(int i=0;i<2000;i++) b.tick(0.05f);
  b.detener(); float t=0;
  while(b.rpmActual()>0.0f && t<20.f){ b.tick(0.05f); t+=0.05f; }
  printf("\n  Frenado desde 100 RPM -> 0: %.2f s   (FRENADO_PARADA_RPM_S=%.1f)\n", t, (double)FRENADO_PARADA_RPM_S);
  return 0;
}
