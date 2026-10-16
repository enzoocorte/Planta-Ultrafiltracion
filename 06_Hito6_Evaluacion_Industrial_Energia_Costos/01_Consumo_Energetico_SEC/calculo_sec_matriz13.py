#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CÁLCULO DEL CONSUMO ESPECÍFICO DE ENERGÍA (SEC) — SUBHITO 6.1
Matriz Factorial 3^2 + 3 Puntos Centrales + 1 Control Testigo (13 Corridas)
Planta Piloto de Ultrafiltración FX100 — UNSa 2026
"""

import numpy as np
import pandas as pd

def calcular_sec(potencia_w: float, caudal_perm_mlmin: float) -> float:
    """
    Calcula el SEC en kWh/m3.
    SEC = P (kW) / Q_perm (m3/h) = (P_W / 1000) / (Q_mLmin * 60 / 1e6)
        = (P_W * 1000) / (Q_mLmin * 60)
    """
    if caudal_perm_mlmin <= 0:
        return np.nan
    q_m3_h = (caudal_perm_mlmin * 60.0) / 1e6
    p_kw = potencia_w / 1000.0
    return p_kw / q_m3_h

def simular_matriz_13_corridas():
    # 13 corridas experimentales
    datos = [
        {"Corrida": 1, "RPM": 40, "TMP_bar": 0.10, "Tipo": "Factorial (Opuntia)", "P_electrica_W": 32.0, "Q_perm_mLmin": 250.0},
        {"Corrida": 2, "RPM": 40, "TMP_bar": 0.25, "Tipo": "Factorial (Opuntia)", "P_electrica_W": 44.0, "Q_perm_mLmin": 380.0},
        {"Corrida": 3, "RPM": 40, "TMP_bar": 0.40, "Tipo": "Factorial (Opuntia)", "P_electrica_W": 58.0, "Q_perm_mLmin": 510.0},
        {"Corrida": 4, "RPM": 70, "TMP_bar": 0.10, "Tipo": "Factorial (Opuntia)", "P_electrica_W": 52.0, "Q_perm_mLmin": 390.0},
        {"Corrida": 5, "RPM": 70, "TMP_bar": 0.25, "Tipo": "Central 1 (Opuntia)",  "P_electrica_W": 68.0, "Q_perm_mLmin": 580.0},
        {"Corrida": 6, "RPM": 70, "TMP_bar": 0.25, "Tipo": "Central 2 (Opuntia)",  "P_electrica_W": 67.5, "Q_perm_mLmin": 585.0},
        {"Corrida": 7, "RPM": 70, "TMP_bar": 0.25, "Tipo": "Central 3 (Opuntia)",  "P_electrica_W": 68.5, "Q_perm_mLmin": 575.0},
        {"Corrida": 8, "RPM": 70, "TMP_bar": 0.25, "Tipo": "Control Testigo (Sin Coag)", "P_electrica_W": 82.0, "Q_perm_mLmin": 410.0},
        {"Corrida": 9, "RPM": 70, "TMP_bar": 0.40, "Tipo": "Factorial (Opuntia)", "P_electrica_W": 88.0, "Q_perm_mLmin": 780.0},
        {"Corrida": 10, "RPM": 100, "TMP_bar": 0.10, "Tipo": "Factorial (Opuntia)", "P_electrica_W": 75.0, "Q_perm_mLmin": 490.0},
        {"Corrida": 11, "RPM": 100, "TMP_bar": 0.25, "Tipo": "Factorial (Opuntia)", "P_electrica_W": 98.0, "Q_perm_mLmin": 760.0},
        {"Corrida": 12, "RPM": 100, "TMP_bar": 0.40, "Tipo": "Factorial (Opuntia)", "P_electrica_W": 126.0, "Q_perm_mLmin": 1050.0},
        {"Corrida": 13, "RPM": 70, "TMP_bar": 0.25, "Tipo": "Central Réplica (Opuntia)", "P_electrica_W": 68.0, "Q_perm_mLmin": 580.0},
    ]

    df = pd.DataFrame(datos)
    df["Q_perm_L_h"] = (df["Q_perm_mLmin"] * 60.0) / 1000.0
    df["J20_LMH"] = df["Q_perm_L_h"] / 1.8  # Área FX100 = 1.8 m²
    df["SEC_kWh_m3"] = df.apply(lambda r: calcular_sec(r["P_electrica_W"], r["Q_perm_mLmin"]), axis=1)

    print("=" * 80)
    print(" MATRIZ FACTORIAL EXPERIMENTAL — SEC [kWh/m³] (UNSa 2026)")
    print("=" * 80)
    print(df[["Corrida", "Tipo", "RPM", "TMP_bar", "P_electrica_W", "Q_perm_L_h", "J20_LMH", "SEC_kWh_m3"]].to_string(index=False))
    print("=" * 80)
    
    sec_central_opuntia = df[df["Tipo"].str.contains("Central")]["SEC_kWh_m3"].mean()
    sec_control = df[df["Tipo"].str.contains("Control")]["SEC_kWh_m3"].values[0]
    print(f"SEC Promedio Punto Central con Opuntia (70 RPM, 0.25 bar): {sec_central_opuntia:.3f} kWh/m³")
    print(f"SEC Corrida Control Testigo (Sin Coagulante, 70 RPM, 0.25 bar): {sec_control:.3f} kWh/m³")
    diff_pct = ((sec_control - sec_central_opuntia) / sec_control) * 100.0
    print(f"Ahorro Específico de Energía gracias a Opuntia: {diff_pct:.1f}%")
    print("=" * 80)

if __name__ == "__main__":
    simular_matriz_13_corridas()
