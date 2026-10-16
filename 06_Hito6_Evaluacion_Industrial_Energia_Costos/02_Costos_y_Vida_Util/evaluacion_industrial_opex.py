#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
EVALUACIÓN INDUSTRIAL DE COSTOS OPERATIVOS (OPEX) Y VIDA ÚTIL — SUBHITO 6.2
Planta Piloto de Ultrafiltración FX100 (Helixone® 1.8 m²) — UNSa 2026
Autor: Enzo Corte & Tesistas (Antonella Guitián / Owen Cañizares)
"""

import numpy as np
import pandas as pd

def calcular_opex(
    sec_kwh_m3: float = 0.28,
    tarifa_kwh_usd: float = 0.10,
    dosis_coag_g_m3: float = 40.0,
    costo_coag_kg_usd: float = 0.60,
    vol_cip_l: float = 4.0,
    intervalo_cip_h: float = 100.0,
    caudal_medio_lh: float = 35.0,
    costo_cip_evento_usd: float = 0.22,
    costo_cartucho_fx100_usd: float = 55.0,
    horas_vida_util_h: float = 3000.0
):
    """
    Calcula el desglose del costo operativo unitario (OPEX) en USD/m3.
    """
    # 1. Costo energético
    c_energia = sec_kwh_m3 * tarifa_kwh_usd

    # 2. Costo de coagulante
    c_coagulante = (dosis_coag_g_m3 / 1000.0) * costo_coag_kg_usd

    # 3. Costo de limpieza CIP
    vol_ciclo_m3 = (intervalo_cip_h * caudal_medio_lh) / 1000.0
    c_cip = costo_cip_evento_usd / vol_ciclo_m3 if vol_ciclo_m3 > 0 else 0.0

    # 4. Amortización / Reposición del módulo de ultrafiltración FX100
    vol_total_m3 = (horas_vida_util_h * caudal_medio_lh) / 1000.0
    c_reposicion = costo_cartucho_fx100_usd / vol_total_m3 if vol_total_m3 > 0 else 0.0

    opex_total = c_energia + c_coagulante + c_cip + c_reposicion

    return {
        "C_Energia_USD_m3": round(c_energia, 4),
        "C_Coagulante_USD_m3": round(c_coagulante, 4),
        "C_CIP_USD_m3": round(c_cip, 4),
        "C_Reposicion_USD_m3": round(c_reposicion, 4),
        "OPEX_Total_USD_m3": round(opex_total, 4),
        "Vol_Tratado_Ciclo_m3": round(vol_ciclo_m3, 2),
        "Vol_Total_Vida_m3": round(vol_total_m3, 2)
    }

def main():
    print("=" * 70)
    print(" EVALUACIÓN ECONÓMICA Y COMPARATIVA DE OPEX (HITO 6 - UNSa 2026)")
    print("=" * 70)

    # Escenario A: Control Testigo (Sin Coagulante, Ensuc. Acelerado)
    res_testigo = calcular_opex(
        sec_kwh_m3=0.54,
        tarifa_kwh_usd=0.10,
        dosis_coag_g_m3=0.0,
        costo_coag_kg_usd=0.0,
        vol_cip_l=4.0,
        intervalo_cip_h=15.0,
        caudal_medio_lh=30.0,
        costo_cip_evento_usd=0.22,
        costo_cartucho_fx100_usd=55.0,
        horas_vida_util_h=750.0
    )

    # Escenario B: Con Opuntia ficus-indica (Torta de sacrificio porosa)
    res_opuntia = calcular_opex(
        sec_kwh_m3=0.28,
        tarifa_kwh_usd=0.10,
        dosis_coag_g_m3=40.0,
        costo_coag_kg_usd=0.60,
        vol_cip_l=4.0,
        intervalo_cip_h=100.0,
        caudal_medio_lh=35.0,
        costo_cip_evento_usd=0.22,
        costo_cartucho_fx100_usd=55.0,
        horas_vida_util_h=3000.0
    )

    df = pd.DataFrame([res_testigo, res_opuntia], index=["Control Sin Coagulante", "Con Opuntia ficus-indica"])
    print(df.T)
    print("\n" + "=" * 70)
    ahorro_pct = ((res_testigo["OPEX_Total_USD_m3"] - res_opuntia["OPEX_Total_USD_m3"]) / res_testigo["OPEX_Total_USD_m3"]) * 100.0
    print(f"Reducción Global de Costo Unitario con Opuntia: {ahorro_pct:.1f}%")
    print("=" * 70)

if __name__ == "__main__":
    main()
