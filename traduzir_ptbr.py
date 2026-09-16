"""Gera o language_pt_br.h da tela DWIN: entradas antigas SEM acento (a
fonte da tela nao tem glifos acentuados) + as chaves que a ProUI usa e o
pt_br nao tinha. Roda em cima do arquivo original do Marlin."""
import re
import unicodedata

CAMINHO = "Marlin/src/lcd/language/language_pt_br.h"

# ---- chaves que faltavam (ProUI) — sem acento, curtas por causa da tela ----
NOVAS = r'''
  // ---- ProUI (mriscoc): chaves que o pt_br nao tinha — traducao PT-BR ----
  LSTR MSG_115K_BAUD                      = _UxGT("115K baud");
  LSTR MSG_3D_PRINTER                     = _UxGT("Impressora 3D");
  LSTR MSG_ACTIVATE_MESH                  = _UxGT("Ativar nivelamento");
  LSTR MSG_ADVANCED_PAUSE                 = _UxGT("Pausa avancada");
  LSTR MSG_AMAX_A                         = _UxGT("Acel max ") STR_A;
  LSTR MSG_AMAX_B                         = _UxGT("Acel max ") STR_B;
  LSTR MSG_AMAX_C                         = _UxGT("Acel max ") STR_C;
  LSTR MSG_AMAX_E                         = _UxGT("Acel max E");
  LSTR MSG_AMAX_EN                        = _UxGT("Acel max *");
  LSTR MSG_AUTO_MESH                      = _UxGT("Malha automatica");
  LSTR MSG_BAD_HEATER_ID                  = _UxGT("Extrusor invalido.");
  LSTR MSG_BED_IS_RUN                     = _UxGT("da MESA em andamento.");
  LSTR MSG_BED_PID_SETTINGS               = _UxGT("Ajustes PID mesa");
  LSTR MSG_BED_TEMPERATURE                = _UxGT("Temperatura da mesa");
  LSTR MSG_BED_TRAMMING_MANUAL            = _UxGT("Nivelar cantos");
  LSTR MSG_BRIGHTNESS                     = _UxGT("Brilho da tela");
  LSTR MSG_BRIGHTNESS_OFF                 = _UxGT("Apagar tela");
  LSTR MSG_BUTTON_ADVANCED                = _UxGT("Avancado");
  LSTR MSG_BUTTON_CONFIRM                 = _UxGT("Confirmar");
  LSTR MSG_BUTTON_CONTINUE                = _UxGT("Continuar");
  LSTR MSG_BUTTON_MEDIA                   = _UxGT("Cartao");
  LSTR MSG_BUTTON_PAUSE                   = _UxGT("Pausar");
  LSTR MSG_BUTTON_PURGE                   = _UxGT("Purgar");
  LSTR MSG_BUTTON_RESUME                  = _UxGT("Retomar");
  LSTR MSG_BUTTON_SAVE                    = _UxGT("Salvar");
  LSTR MSG_CHECK_FILENAME                 = _UxGT("Confira os nomes dos arquivos");
  LSTR MSG_COLORS_APPLIED                 = _UxGT("Cores aplicadas");
  LSTR MSG_COLORS_BLUE                    = _UxGT("Azul");
  LSTR MSG_COLORS_GET                     = _UxGT("Obter cor");
  LSTR MSG_COLORS_GREEN                   = _UxGT("Verde");
  LSTR MSG_COLORS_RED                     = _UxGT("Vermelho");
  LSTR MSG_COLORS_SELECT                  = _UxGT("Escolher cores");
  LSTR MSG_COLORS_WHITE                   = _UxGT("Branco");
  LSTR MSG_CONTINUE_PRINT_JOB             = _UxGT("Continuar impressao");
  LSTR MSG_ENABLE_HS_MODE                 = _UxGT("Ativar modo HS");
  LSTR MSG_ENDSTOP_DIAGNOSTICS            = _UxGT("Diag. fins de curso");
  LSTR MSG_ENDSTOP_TEST                   = _UxGT("Testar fins de curso");
  LSTR MSG_ENGRAVING                      = _UxGT("Gravando...");
  LSTR MSG_ERROR                          = _UxGT("Erro");
  LSTR MSG_EXTRUDER_MIN_TEMP              = _UxGT("Temp. min extrusor");
  LSTR MSG_FILAMENT_CHANGE_PURGE_CONTINUE = _UxGT("Purgar ou continuar?");
  LSTR MSG_FILAMENT_MAN                   = _UxGT("Gerenciar filamento");
  LSTR MSG_FILAMENT_PARK_ENABLED          = _UxGT("Estacionar bico");
  LSTR MSG_FILAMENT_SET                   = _UxGT("Ajustes de filamento");
  LSTR MSG_FWRETRACT                      = _UxGT("Retracao firmware");
  LSTR MSG_HAS_PREVIEW                    = _UxGT("Tem previa");
  LSTR MSG_HIGH                           = _UxGT("ALTA");
  LSTR MSG_HOME_OFFSET_X                  = _UxGT("Desloc. origem X");
  LSTR MSG_HOME_OFFSET_Y                  = _UxGT("Desloc. origem Y");
  LSTR MSG_HOME_OFFSET_Z                  = _UxGT("Desloc. origem Z");
  LSTR MSG_HOME_Z_AND_DISABLE             = _UxGT("Origem Z e desligar");
  LSTR MSG_HOMING                         = _UxGT("Indo a origem");
  LSTR MSG_HOMING_FEEDRATE                = _UxGT("Veloc. da origem");
  LSTR MSG_HOST_SHUTDOWN                  = _UxGT("Desligar host");
  LSTR MSG_HOTEND_PID_SETTINGS            = _UxGT("Ajustes PID bico");
  LSTR MSG_INFO_BUILD                     = _UxGT("Info da build");
  LSTR MSG_INFO_FWVERSION                 = _UxGT("Versao do firmware");
  LSTR MSG_INFO_MACHINENAME               = _UxGT("Nome da maquina");
  LSTR MSG_INFO_PRINT_COUNT_RESET         = _UxGT("Zerar contagem");
  LSTR MSG_INFO_SIZE                      = _UxGT("Tamanho");
  LSTR MSG_INPUT_SHAPING                  = _UxGT("Input Shaping");
  LSTR MSG_INVERT_EXTRUDER                = _UxGT("Inverter extrusor");
  LSTR MSG_LASER_ENGRAVING                = _UxGT("Gravacao a laser");
  LSTR MSG_LASER_FIRST_HOME               = _UxGT("Faca a origem antes");
  LSTR MSG_LASER_FOCUS                    = _UxGT("Foco do laser");
  LSTR MSG_LASER_MENU                     = _UxGT("Controle do laser");
  LSTR MSG_LASER_NOT_AVAILABLE            = _UxGT("Indisponivel no modo laser");
  LSTR MSG_LASER_RUN_RANGE                = _UxGT("Percorrer area");
  LSTR MSG_LASER_TOGGLE                   = _UxGT("Ligar/desligar laser");
  LSTR MSG_LIVE_MOVE                      = _UxGT("Movimento ao vivo");
  LSTR MSG_LOCKSCREEN                     = _UxGT("Bloquear tela");
  LSTR MSG_LOCKSCREEN_LOCKED              = _UxGT("Impressora bloqueada,");
  LSTR MSG_LOCKSCREEN_UNLOCK              = _UxGT("gire para desbloquear.");
  LSTR MSG_LOW                            = _UxGT("BAIXA");
  LSTR MSG_M48_OUT_OF_BOUNDS              = _UxGT("Sonda fora da area");
  LSTR MSG_MANUAL_MESH                    = _UxGT("Malha manual");
  LSTR MSG_MEDIA_NOT_INSERTED             = _UxGT("Nenhum cartao inserido.");
  LSTR MSG_MEDIA_SORT                     = _UxGT("Ordenar cartao");
  LSTR MSG_MESH_ACTIVE                    = _UxGT("Malha %i ativa");
  LSTR MSG_MESH_AMAX                      = _UxGT("Area maxima");
  LSTR MSG_MESH_CENTER                    = _UxGT("Centralizar area");
  LSTR MSG_MESH_INSET                     = _UxGT("Margem da malha");
  LSTR MSG_MESH_MAX_X                     = _UxGT("Malha X maximo");
  LSTR MSG_MESH_MAX_Y                     = _UxGT("Malha Y maximo");
  LSTR MSG_MESH_MIN_X                     = _UxGT("Malha X minimo");
  LSTR MSG_MESH_MIN_Y                     = _UxGT("Malha Y minimo");
  LSTR MSG_MESH_POINTS_X                  = _UxGT("Pontos da malha X");
  LSTR MSG_MESH_POINTS_Y                  = _UxGT("Pontos da malha Y");
  LSTR MSG_MESH_RESET                     = _UxGT("Malha zerada");
  LSTR MSG_MESH_VIEW                      = _UxGT("Ver malha");
  LSTR MSG_MESH_VIEWER                    = _UxGT("Visualizador de malha");
  LSTR MSG_MOVE_E_100                     = _UxGT("Mover ext. 100mm");
  LSTR MSG_MOVE_Z_HOME                    = _UxGT("Z para a origem");
  LSTR MSG_MPC_AMBIENT_XFER_COEFF         = _UxGT("Coef. ambiente");
  LSTR MSG_MPC_AMBIENT_XFER_COEFF_FAN     = _UxGT("Coef. ventoinha");
  LSTR MSG_MPC_AUTOTUNE                   = _UxGT("Autoajuste MPC");
  LSTR MSG_MPC_BLOCK_HEAT_CAPACITY        = _UxGT("Capacidade termica");
  LSTR MSG_MPC_POWER                      = _UxGT("Potencia aquecedor");
  LSTR MSG_MPC_SETTINGS                   = _UxGT("Ajustes MPC");
  LSTR MSG_MPC_TARGET                     = _UxGT("Alvo MPC:      Celsius");
  LSTR MSG_NOZZLE_IS_RUN                  = _UxGT("do BICO em andamento.");
  LSTR MSG_NOZZLE_TEMPERATURE             = _UxGT("Temperatura do bico");
  LSTR MSG_ONLY_GCODE                     = _UxGT("So imprime arquivos G-code");
  LSTR MSG_OPTION_DISABLED                = _UxGT("Opcao desativada");
  LSTR MSG_OUTAGE_RECV_1                  = _UxGT("Parece que a ultima");
  LSTR MSG_OUTAGE_RECV_2                  = _UxGT("impressao foi interrompida.");
  LSTR MSG_PARK_XPOSITION                 = _UxGT("Posicao X de parada");
  LSTR MSG_PARK_YPOSITION                 = _UxGT("Posicao Y de parada");
  LSTR MSG_PARK_ZRAISE                    = _UxGT("Subida Z na parada");
  LSTR MSG_PAUSE_PRINT_PARKING            = _UxGT(MSG_1_LINE("Estacionando..."));
  LSTR MSG_PHY_SET                        = _UxGT("Ajustes fisicos");
  LSTR MSG_PHY_XBEDSIZE                   = _UxGT("Tamanho X da mesa");
  LSTR MSG_PHY_XMAXPOS                    = _UxGT("Posicao max X");
  LSTR MSG_PHY_XMINPOS                    = _UxGT("Posicao min X");
  LSTR MSG_PHY_YBEDSIZE                   = _UxGT("Tamanho Y da mesa");
  LSTR MSG_PHY_YMAXPOS                    = _UxGT("Posicao max Y");
  LSTR MSG_PHY_YMINPOS                    = _UxGT("Posicao min Y");
  LSTR MSG_PHY_ZMAXPOS                    = _UxGT("Posicao max Z");
  LSTR MSG_PID_AUTOTUNE                   = _UxGT("Autoajuste PID");
  LSTR MSG_PID_AUTOTUNE_FAILED            = _UxGT("Autoajuste falhou!");
  LSTR MSG_PID_CYCLE                      = _UxGT("Ciclos PID");
  LSTR MSG_PID_SET_KD                     = _UxGT("Definir" STR_KD);
  LSTR MSG_PID_SET_KI                     = _UxGT("Definir" STR_KI);
  LSTR MSG_PID_SET_KP                     = _UxGT("Definir" STR_KP);
  LSTR MSG_PID_TARGET                     = _UxGT("Alvo PID:      Celsius");
  LSTR MSG_PID_TIMEOUT                    = _UxGT("Autoajuste falhou! Tempo esgotado.");
  LSTR MSG_PLEASE_PREHEAT                 = _UxGT("Preaqueca o bico, por favor.");
  LSTR MSG_PLEASE_WAIT                    = _UxGT("Aguarde...");
  LSTR MSG_PLEASE_WAIT_REBOOT             = _UxGT("Aguarde a reinicializacao.");
  LSTR MSG_POSITION_IS_UNKNOWN            = _UxGT("AVISO: posicao desconhecida, faca a origem");
  LSTR MSG_PREHEAT_HOTEND                 = _UxGT("Preaquecer bico");
  LSTR MSG_PRINTER_KILLED                 = _UxGT("Impressora parada!");
  LSTR MSG_PRINT_DONE                     = _UxGT("Impressao concluida");
  LSTR MSG_PROBE_WIZARD                   = _UxGT("Assistente sonda Z");
  LSTR MSG_PROBE_WIZARD_MOVING            = _UxGT("Indo ate a posicao");
  LSTR MSG_PROBE_WIZARD_PROBING           = _UxGT("Sondando referencia Z");
  LSTR MSG_PROUIEX_SET                    = _UxGT("Ajustes ProUIex");
  LSTR MSG_REMAINING_TIME                 = _UxGT("Restante");
  LSTR MSG_RESET_STATS                    = _UxGT("Zerar estatisticas?");
  LSTR MSG_RUNOUT_ACTIVE                  = _UxGT("Sensor filam. ativo");
  LSTR MSG_RUNOUT_DISTANCE_MM             = _UxGT("Dist. fim filam. mm");
  LSTR MSG_RUNOUT_ENABLE                  = _UxGT("Ativar sensor filam.");
  LSTR MSG_SENSOR_RESPONSIVENESS          = _UxGT("Resposta do sensor");
  LSTR MSG_SET_AS_HOME                    = _UxGT("Definir como origem");
  LSTR MSG_SHAPING_A_FREQ                 = STR_A _UxGT(" frequencia");
  LSTR MSG_SHAPING_A_ZETA                 = STR_A _UxGT(" amortecimento");
  LSTR MSG_SHAPING_B_FREQ                 = STR_B _UxGT(" frequencia");
  LSTR MSG_SHAPING_B_ZETA                 = STR_B _UxGT(" amortecimento");
  LSTR MSG_SHAPING_C_FREQ                 = STR_C _UxGT(" frequencia");
  LSTR MSG_SHAPING_C_ZETA                 = STR_C _UxGT(" amortecimento");
  LSTR MSG_SINGLENOZZLE_UNRETRACT_SPEED   = _UxGT("Veloc. de retorno");
  LSTR MSG_SOUND_ENABLE                   = _UxGT("Ativar som");
  LSTR MSG_STEP_SMOOTHING                 = _UxGT("Suavizar passos");
  LSTR MSG_TMC_ACURRENT                   = STR_A _UxGT(" corrente driver");
  LSTR MSG_TMC_BCURRENT                   = STR_B _UxGT(" corrente driver");
  LSTR MSG_TMC_CCURRENT                   = STR_C _UxGT(" corrente driver");
  LSTR MSG_TMC_DRIVERS                    = _UxGT("Drivers TMC");
  LSTR MSG_TMC_ECURRENT                   = _UxGT("Corrente driver E");
  LSTR MSG_TOOLBAR_SETUP                  = _UxGT("Barra de ferramentas");
  LSTR MSG_TOO_HIGH                       = _UxGT("esta alta demais");
  LSTR MSG_TOO_LOW                        = _UxGT("esta baixa demais");
  LSTR MSG_TRAMMING_WIZARD                = _UxGT("Assistente de cantos");
  LSTR MSG_TRAM_BL                        = _UxGT("Tras esquerda");
  LSTR MSG_TRAM_BR                        = _UxGT("Tras direita");
  LSTR MSG_TRAM_C                         = _UxGT("Centro");
  LSTR MSG_TRAM_FL                        = _UxGT("Frente esquerda");
  LSTR MSG_TRAM_FR                        = _UxGT("Frente direita");
  LSTR MSG_TURN_OFF                       = _UxGT("Desligue a impressora");
  LSTR MSG_UBL_MESH_FILLED                = _UxGT("Pontos faltantes preenchidos");
  LSTR MSG_UBL_MESH_INVALID               = _UxGT("Malha invalida");
  LSTR MSG_UBL_MESH_TILTED                = _UxGT("Malha inclinada");
  LSTR MSG_UBL_TILTING_GRID               = _UxGT("Grade de inclinacao");
  LSTR MSG_UBL_TILT_MESH                  = _UxGT("Inclinar malha");
  LSTR MSG_VMAX_A                         = _UxGT("Veloc max ") STR_A;
  LSTR MSG_VMAX_B                         = _UxGT("Veloc max ") STR_B;
  LSTR MSG_VMAX_C                         = _UxGT("Veloc max ") STR_C;
  LSTR MSG_VMAX_E                         = _UxGT("Veloc max E");
  LSTR MSG_ZPROBE_MULTIPLE                = _UxGT("Sondagem multipla");
  LSTR MSG_ZPROBE_SETTINGS                = _UxGT("Ajustes da sonda");
  LSTR MSG_Z_AFTER_HOME                   = _UxGT("Z apos a origem");
  LSTR MSG_Z_FEED_RATE                    = _UxGT("Velocidade Z");
  LSTR MSG_Z_POSITION_IS_UNKNOWN          = _UxGT("AVISO: posicao Z desconhecida, leve Z a origem");
'''


def sem_acento(texto: str) -> str:
    """Tira acentos/cedilha e simbolos fora do ASCII que a fonte DWIN nao tem."""
    saida = []
    for ch in texto:
        if ord(ch) < 128:
            saida.append(ch)
            continue
        base = unicodedata.normalize("NFKD", ch)
        base = "".join(c for c in base if ord(c) < 128)
        if base:
            saida.append(base)
        elif ch in "°º":
            saida.append("")          # grau: a tela ja desenha o simbolo por icone
        else:
            saida.append("?")         # nao deveria acontecer; fica visivel no teste
    return "".join(saida)


def main():
    src = open(CAMINHO, encoding="utf-8").read()
    if "ProUI (mriscoc): chaves que o pt_br nao tinha" in src:
        raise SystemExit("ja traduzido")

    # 1) tira acento so' DENTRO dos literais de string
    src2 = re.sub(r'"((?:[^"\\]|\\.)*)"', lambda m: '"' + sem_acento(m.group(1)) + '"', src)

    # 2) insere as chaves novas no fim do namespace LanguageNarrow_pt_br
    marca = "namespace LanguageNarrow_pt_br {"
    ini = src2.index(marca)
    fim = src2.index("\n}\n", ini)            # primeiro fechamento depois do namespace
    src2 = src2[:fim] + "\n" + NOVAS.rstrip("\n") + src2[fim:]

    # 3) nome do idioma
    src2 = src2.replace('_UxGT("Portuguese (BR)")', '_UxGT("Portugues (BR)")')
    open(CAMINHO, "w", encoding="utf-8").write(src2)
    restos = [l for l in src2.splitlines() if any(ord(c) > 127 for c in l)]
    print("linhas ainda com nao-ASCII:", len(restos))
    for l in restos[:10]:
        print("  ", l.strip()[:100])
    print("ok:", CAMINHO)


if __name__ == "__main__":
    main()
