"""Troca a configuracao do firmware entre as duas variantes da Ender 3 V2 (placa 4.2.2):

    python trocar_config.py mm     -> malha manual (sem sonda)
    python trocar_config.py blt    -> BLTouch / CRTouch / 3DTouch (malha automatica)

Copia Configuration.h, Configuration_adv.h e Version.h ORIGINAIS do mriscoc
(pasta configurations/) para Marlin/ e reaplica os nossos ajustes. O codigo
fonte e' o mesmo para as duas; so' estes tres arquivos mudam. Depois de
trocar, compile normalmente (pio run -e STM32F103RC_creality).
"""
import shutil
import sys

VARIANTES = {"mm": "configurations/Ender3V2-422-MM/", "blt": "configurations/Ender3V2-422-BLT/"}

# (texto original, texto novo) — cada um tem que aparecer exatamente 1 vez
AJUSTES = [
    (b"#define INVERT_E0_DIR false",
     b"#define INVERT_E0_DIR true   // Ricardo: extrusora invertida por padrao"),
    (b"//#define RESTORE_LEVELING_AFTER_G28",      # a malha volta sozinha depois do G28
     b"#define RESTORE_LEVELING_AFTER_G28"),
    (b"#define LCD_LANGUAGE en",
     b"#define LCD_LANGUAGE pt_br"),
]

# idem para Configuration_adv.h
AJUSTES_ADV = [
    # compensacao de folga do eixo Z (medida: 0,2 mm). Ajustavel por M425 (ex.: M425 Z0.15, M500).
    (b"//#define BACKLASH_COMPENSATION",
     b"#define BACKLASH_COMPENSATION   // Ricardo: folga de 0,2 mm medida no Z; ajuste com M425"),
    (b"#define BACKLASH_DISTANCE_MM { 0, 0, 0 }",
     b"#define BACKLASH_DISTANCE_MM { 0, 0, 0.2 }   /* Ricardo: Z medido */"),
    (b"  #define BACKLASH_CORRECTION    0.0       // 0.0 = no correction; 1.0 = full correction",
     b"  #define BACKLASH_CORRECTION    1.0       // 0.0 = no correction; 1.0 = full correction  // Ricardo: correcao total"),
    (b"  //#define BACKLASH_GCODE",
     b"  #define BACKLASH_GCODE   // Ricardo: M425 para ajustar/salvar na EEPROM"),
    # nao medir folga a cada toque da sonda (deixa a sondagem mais lenta); o valor vem do menu/M425
    (b"    #define MEASURE_BACKLASH_WHEN_PROBING",
     b"    //#define MEASURE_BACKLASH_WHEN_PROBING   // Ricardo: desligado, sondagem mais rapida"),
]


def main():
    if len(sys.argv) != 2 or sys.argv[1].lower() not in VARIANTES:
        raise SystemExit(__doc__)
    nome = sys.argv[1].lower()
    origem = VARIANTES[nome]

    cfg = open(origem + "Configuration.h", "rb").read()
    for antigo, novo in AJUSTES:
        if cfg.count(antigo) != 1:
            raise SystemExit("ajuste nao encontrado (ou repetido) em Configuration.h: %r" % antigo)
        cfg = cfg.replace(antigo, novo)
    open("Marlin/Configuration.h", "wb").write(cfg)
    adv = open(origem + "Configuration_adv.h", "rb").read()
    for antigo, novo in AJUSTES_ADV:
        if adv.count(antigo) != 1:
            raise SystemExit("ajuste nao encontrado (ou repetido) em Configuration_adv.h: %r" % antigo)
        adv = adv.replace(antigo, novo)
    open("Marlin/Configuration_adv.h", "wb").write(adv)
    shutil.copyfile(origem + "Version.h", "Marlin/Version.h")
    print("config ativa:", nome.upper(), "<-", origem)


if __name__ == "__main__":
    main()
