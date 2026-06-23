import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Configuração da janela e do gráfico 3D
fig = plt.figure(figsize=(12, 10))
ax = fig.add_subplot(111, projection='3d')

# Criando o "Aquário 3D" (8000 neurônios espalhados no espaço X, Y, Theta)
res = 20 # Resolução da grade
x_grid = np.linspace(-5, 5, res)
y_grid = np.linspace(-5, 5, res)
z_grid = np.linspace(0, 360, res) # Z agora é o Theta (Orientação de 0 a 360)
X_grid, Y_grid, Z_grid = np.meshgrid(x_grid, y_grid, z_grid)

# Vetor com a coordenada de todos os neurônios
neuronios = np.vstack([X_grid.ravel(), Y_grid.ravel(), Z_grid.ravel()]).T

# A Gaussiana 3D com a regra do Toroide (Pac-Man) para o ângulo
def gaussiana_3d(pontos, xc, yc, zc, forca=1.0, tam_xy=1.0, tam_z=45.0):
    dx = pontos[:, 0] - xc
    dy = pontos[:, 1] - yc
    
    # Efeito Toroide
    dz = np.abs(pontos[:, 2] - zc)
    dz = np.minimum(dz, 360 - dz)
    
    # Equação da energia 3D
    energia = forca * np.exp(-(dx**2 + dy**2) / (2 * tam_xy**2) - (dz**2) / (2 * tam_z**2))
    return energia

# Parâmetros da simulação
frames_loop = 200  # Volta completa
frames_wta = 100    # Briga Winner-Takes-All
total_frames = frames_loop + frames_wta

# Históricos para desenhar a linha do trajeto
hist_x_real, hist_y_real, hist_z_real = [], [], []
hist_x_odo, hist_y_odo, hist_z_odo = [], [], []

info_text = fig.text(0.05, 0.05, '', fontsize=11, bbox=dict(facecolor='white', alpha=0.9, edgecolor='black'))

def atualizar(frame):
    ax.clear()
    
    # Limites do aquário
    ax.set_xlim(-5, 5)
    ax.set_ylim(-5, 5)
    ax.set_zlim(0, 360)
    ax.set_xlabel('Posição X')
    ax.set_ylabel('Posição Y')
    ax.set_zlabel('Orientação Theta (Graus)')
    
    # Visão clássica 3D (Comente se quiser usar top-down)
    ax.view_init(elev=90, azim=-90)

    progresso = min(frame / frames_loop, 1.0)
    angulo_trajeto = progresso * 2 * np.pi
    
    # 1. Posição Real 
    raio_real = 3.0
    x_real = raio_real * np.cos(angulo_trajeto)
    y_real = raio_real * np.sin(angulo_trajeto)
    theta_real = (np.degrees(angulo_trajeto) + 90) % 360
    
    # 2. Odometria 
    erro_pos = progresso * 1.5
    erro_theta = progresso * 40.0 
    
    x_odo = (raio_real + erro_pos) * np.cos(angulo_trajeto)
    y_odo = (raio_real + erro_pos) * np.sin(angulo_trajeto)
    theta_odo = (theta_real - erro_theta) % 360
    
    # 3. ID da Imagem
    imagem_id = int(progresso * 20)
    if imagem_id >= 20: imagem_id = 0
        
    if frame < frames_loop:
        hist_x_real.append(x_real); hist_y_real.append(y_real); hist_z_real.append(theta_real)
        hist_x_odo.append(x_odo); hist_y_odo.append(y_odo); hist_z_odo.append(theta_odo)

    # --- DINÂMICA DA REDE 3D ---
    if frame < frames_loop:
        energia_total = gaussiana_3d(neuronios, x_odo, y_odo, theta_odo, forca=1.0)
        ax.set_title("Pose Cell Network: Odometria empurrando a Nuvem de Energia", fontsize=14)
        info_text.set_text(f"Odometria -> X:{x_odo:.1f}, Y:{y_odo:.1f}, Theta:{theta_odo:.0f}º\nErro Angular: {erro_theta:.0f}º")
    else:
        # FASE 2: Winner-Takes-All 3D com Explosão de Energia
        taxa = (frame - frames_loop) / frames_wta
        
        x_odo_fim = (raio_real + 1.5) * np.cos(2 * np.pi)
        y_odo_fim = (raio_real + 1.5) * np.sin(2 * np.pi)
        theta_odo_fim = (90 - 40) % 360 
        
        x_visao = raio_real * np.cos(0)
        y_visao = raio_real * np.sin(0)
        theta_visao = 90.0 
        
        # --- A MÁGICA DO BRILHO ACONTECE AQUI ---
        f_odo = 1.0 - taxa  # Odometria derrete até 0
        
        # A Visão nasce como uma explosão (força 3.0) e relaxa até o normal (1.0)
        f_vis = 3.0 * (1.0 - taxa) + 1.0 * taxa 
        
        energia_total = gaussiana_3d(neuronios, x_odo_fim, y_odo_fim, theta_odo_fim, forca=f_odo) + \
                        gaussiana_3d(neuronios, x_visao, y_visao, theta_visao, forca=f_vis)
        
        ax.set_title("LOOP CLOSURE! Injeção massiva de energia da Câmera", fontsize=14, color='red', fontweight='bold')
        info_text.set_text(f"PICO DE ENERGIA (Visão): Força {f_vis:.1f}\nEsmagando a odometria errada...\nSaltando para -> X:{x_visao:.1f}, Y:{y_visao:.1f}, Theta:{theta_visao:.0f}º")

    # --- Otimização Visual ---
    mask_ativos = energia_total > 0.15
    n_ativos = neuronios[mask_ativos]
    e_ativos = energia_total[mask_ativos]
    
    # --- A MÁGICA DA ESCALA DE COR AQUI ---
    # vmin=0.0 e vmax=3.0 garantem que a energia 3.0 brilhe em branco/amarelo intenso
    ax.scatter(n_ativos[:,0], n_ativos[:,1], n_ativos[:,2], 
               c=e_ativos, cmap='magma', s=e_ativos*120, alpha=0.8,
               vmin=0.0, vmax=3.0)

    # Plota as trilhas do trajeto no espaço 3D
    ax.plot(hist_x_real, hist_y_real, hist_z_real, color='green', linewidth=2, label="Real")
    ax.plot(hist_x_odo, hist_y_odo, hist_z_odo, color='red', linestyle='--', linewidth=2, label="Odometria")
    
    # Marca a cabeça do robô
    ax.plot([x_real], [y_real], [theta_real], marker='o', color='green', markersize=6)
    ax.legend(loc="upper left")

# Inicia a animação
ani = animation.FuncAnimation(fig, atualizar, frames=total_frames, interval=30)

# =========================================================
# Opções de exibição/salvamento:
plt.show()

# print("Gerando vídeo...")
# ani.save('RatSLAM_3D_Topo.mp4', writer='ffmpeg', fps=30)
# print("Sucesso!")

# print("Gerando GIF...")
# # ani.save('RatSLAM_3D.gif', writer='pillow', fps=30)
# print("Sucesso!")