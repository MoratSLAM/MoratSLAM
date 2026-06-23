import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Configuração da janela e do gráfico 3D
fig = plt.figure(figsize=(12, 8))
ax = fig.add_subplot(111, projection='3d')

# Criando a "cama elástica" (Rede de Células de Pose / Continuous Attractor)
x = np.linspace(-6, 6, 80)
y = np.linspace(-6, 6, 80)
X, Y = np.meshgrid(x, y)

# A função matemática elegante: gerando a Gaussiana
def gaussiana_energia(x, y, x_centro, y_centro, forca=1.0, tamanho=0.8):
    return forca * np.exp(-((x - x_centro)**2 + (y - y_centro)**2) / (2 * tamanho**2))

# Parâmetros da simulação
frames_loop = 220  # Tempo que o robô leva para dar a volta completa
frames_wta = 100    # Tempo da briga "Winner-Takes-All"
total_frames = frames_loop + frames_wta

# Listas para guardar o histórico do trajeto
hist_x_real, hist_y_real = [], []
hist_x_odo, hist_y_odo = [], []

# Caixa de texto estática para acompanhar as variáveis em tempo real
info_text = fig.text(0.05, 0.05, '', fontsize=12, bbox=dict(facecolor='white', alpha=0.9, edgecolor='black'))

def atualizar(frame):
    ax.clear()
    
    # Travando a câmera para a malha não pular durante a animação
    ax.set_zlim(0, 2.0)
    ax.set_xlim(-6, 6)
    ax.set_ylim(-6, 6)
    ax.axis('off')
    
    # Progresso da volta (de 0.0 a 1.0)
    progresso = min(frame / frames_loop, 1.0) 
    theta = progresso * 2 * np.pi # Ângulo da trajetória circular
    
    # 1. Posição Real (Ground Truth: onde o robô de fato está)
    raio_real = 3.0
    x_real = raio_real * np.cos(theta)
    y_real = raio_real * np.sin(theta)
    
    # 2. Odometria (Empurrando a Gaussiana com deriva/erro acumulado)
    erro_acumulado = progresso * 1.8 # Chega a 1.8m de erro no final da volta
    x_odo = (raio_real + erro_acumulado) * np.cos(theta)
    y_odo = (raio_real + erro_acumulado) * np.sin(theta)
    
    # 3. Simulando a Câmera lendo as Imagens Locais (IDs de 0 a 20)
    imagem_id = int(progresso * 20)
    if imagem_id >= 20: 
        imagem_id = 0 # Loop completo, voltou para a imagem inicial
        
    # --- SALVANDO O HISTÓRICO ---
    # Só adicionamos pontos ao histórico enquanto o robô está andando (Fase 1)
    if frame < frames_loop:
        hist_x_real.append(x_real)
        hist_y_real.append(y_real)
        hist_x_odo.append(x_odo)
        hist_y_odo.append(y_odo)

    # --- DINÂMICA DA REDE ATRATORA CONTÍNUA ---
    if frame < frames_loop:
        # FASE 1: Navegação normal (Odometria ativa)
        Z = gaussiana_energia(X, Y, x_odo, y_odo, forca=1.0)
        ax.set_title("Navegação: Odometria empurrando a Gaussiana (acumulando erro)", fontsize=14)
        info_text.set_text(f"STATUS: Explorando trajeto circular\nOdometria atual -> X: {x_odo:.2f}, Y: {y_odo:.2f}\nImagem na câmera: {imagem_id}")
    
    else:
        # FASE 2: Fechamento de Ciclo (Winner-Takes-All via Inibição Global)
        taxa_wta = (frame - frames_loop) / frames_wta # Transição de 0.0 a 1.0
        
        x_odo_final = (raio_real + 1.8) * np.cos(2 * np.pi)
        y_odo_final = (raio_real + 1.8) * np.sin(2 * np.pi)
        
        x_visao = raio_real * np.cos(0)
        y_visao = raio_real * np.sin(0)
        
        forca_odo = 1.0 - taxa_wta    # Odometria perde suporte e decai
        forca_visao = taxa_wta * 1.5  # Visão entra com força superior
        
        Z = gaussiana_energia(X, Y, x_odo_final, y_odo_final, forca=forca_odo) + \
            gaussiana_energia(X, Y, x_visao, y_visao, forca=forca_visao)

        ax.set_title("LOOP CLOSURE! Imagem 0 inibe a Gaussiana errada", fontsize=14, color='red', fontweight='bold')
        info_text.set_text(f"STATUS: Winner-Takes-All (Correção!)\nImagem reconhecida: {imagem_id}\nEsmagando Gaussiana errada em ({x_odo_final:.1f}, {y_odo_final:.1f})\nSaltando para o Atrator: ({x_visao:.1f}, {y_visao:.1f})")

    # Renderiza a malha 3D
    ax.plot_surface(X, Y, Z, cmap='magma', edgecolor='none', alpha=0.8) # alpha=0.8 para ver as linhas através da malha

    # --- DESENHANDO AS TRILHAS NO CHÃO (Z=0) ---
    # Trilha verde (Ground Truth perfeito)
    ax.plot(hist_x_real, hist_y_real, 0, color='green', linewidth=3, label="Trajeto Real")
    # Trilha vermelha pontilhada (Erro da Odometria)
    ax.plot(hist_x_odo, hist_y_odo, 0, color='red', linewidth=2, label="Trajeto Odometria")
    
    # Pontinho verde na "cabeça" do trajeto real
    ax.plot([x_real], [y_real], [0], marker='o', color='green', markersize=4)
    
    # Exibe a legenda para ficar claro qual linha é qual
    ax.legend(loc="upper right")

# Inicia a animação
ani = animation.FuncAnimation(fig, atualizar, frames=total_frames, interval=40)

# ====================== Bloco de plot ou gravação ========================
# Descomente um só bloco para plotar ou gravar

# plot
plt.show()

# MP4
# print("Salvando o vídeo MP4... Isso pode levar alguns segundos.")
# # Salva o arquivo na mesma pasta onde o seu script Python está
# ani.save('RatSLAM_CANN.mp4', writer='ffmpeg', fps=30)
# print("Vídeo salvo com sucesso!")

# GIF
# print("Salvando o GIF... Isso pode levar alguns segundos.")
# ani.save('RatSLAM_CANN.gif', writer='pillow', fps=30)
# print("GIF salvo com sucesso!")