import pandas as pd
import numpy as np
from math import radians, cos, degrees
import folium
from scipy.spatial.distance import directed_hausdorff

# ==========================================
# CONFIGURAÇÕES GLOBAIS
# ==========================================
ANGULO_ROTACAO_GRAUS = 32

# ==========================================
# 1. CARREGAR OS DADOS E REMOVER APENAS OUTLIERS
# ==========================================
gps_df = pd.read_csv('gps.csv')
med_lat = gps_df['latitude'].median()
med_lon = gps_df['longitude'].median()
filtro_outliers = (np.abs(gps_df['latitude'] - med_lat) < 0.5) & \
                  (np.abs(gps_df['longitude'] - med_lon) < 0.5)
gps_df = gps_df[filtro_outliers].copy()
gps_df.reset_index(drop=True, inplace=True)

top_df = pd.read_csv('nodes.csv')
top_df.rename(columns={'id': 'node_id'}, inplace=True)

if 'node_count' in top_df.columns:
    maior_count = top_df['node_count'].max()
    top_df = top_df[top_df['node_count'] == maior_count].copy()
    
top_df.reset_index(drop=True, inplace=True)

# ==========================================
# 2. CONVERTER GPS PARA METROS (LOCAL) E ALINHAR ORIGENS (0, 0)
# ==========================================
R = 6378137 
lat0_rad = radians(gps_df['latitude'].iloc[0])
lon0_rad = radians(gps_df['longitude'].iloc[0])

gps_x, gps_y = [], []
for index, row in gps_df.iterrows():
    lat_rad = radians(row['latitude'])
    lon_rad = radians(row['longitude'])
    x = R * cos(lat0_rad) * (lon_rad - lon0_rad)
    y = R * (lat_rad - lat0_rad)
    gps_x.append(x)
    gps_y.append(y)

gps_df['x'] = gps_x
gps_df['y'] = gps_y

gps_df['x'] -= gps_df['x'].iloc[0]
gps_df['y'] -= gps_df['y'].iloc[0]

top_df['x'] -= top_df['x'].iloc[0]
top_df['y'] -= top_df['y'].iloc[0]

# ==========================================
# 3. ROTACIONAR RATSLAM COM BASE NO ÂNGULO FORNECIDO
# ==========================================
rotation_angle = radians(ANGULO_ROTACAO_GRAUS)
cos_theta = np.cos(rotation_angle)
sin_theta = np.sin(rotation_angle)

top_df['aligned_x'] = top_df['x'] * cos_theta - top_df['y'] * sin_theta
top_df['aligned_y'] = top_df['x'] * sin_theta + top_df['y'] * cos_theta

# ==========================================
# 4. CÁLCULO DA DISTÂNCIA DE HAUSDORFF E CAPTURA DOS ÍNDICES
# ==========================================
coords_gps = np.column_stack((gps_df['x'], gps_df['y']))
coords_ratslam = np.column_stack((top_df['aligned_x'], top_df['aligned_y']))

hausdorff_ab_val, idx_gps_in_ab, idx_ratslam_in_ab = directed_hausdorff(coords_gps, coords_ratslam)
hausdorff_ba_val, idx_ratslam_in_ba, idx_gps_in_ba = directed_hausdorff(coords_ratslam, coords_gps)

if hausdorff_ab_val >= hausdorff_ba_val:
    distancia_hausdorff = hausdorff_ab_val
    idx_max_gps = idx_gps_in_ab
    idx_max_ratslam = idx_ratslam_in_ab
else:
    distancia_hausdorff = hausdorff_ba_val
    idx_max_gps = idx_gps_in_ba
    idx_max_ratslam = idx_ratslam_in_ba

print(f"Distância de Hausdorff calculada: {distancia_hausdorff:.2f} metros")

# ==========================================
# 5. CONVERTER RATSLAM ROTACIONADO DE VOLTA PARA LAT/LON
# ==========================================
top_lat, top_lon = [], []
for index, row in top_df.iterrows():
    lat_calc_rad = lat0_rad + (row['aligned_y'] / R)
    lon_calc_rad = lon0_rad + (row['aligned_x'] / (R * cos(lat0_rad)))
    top_lat.append(degrees(lat_calc_rad))
    top_lon.append(degrees(lon_calc_rad))

top_df['latitude'] = top_lat
top_df['longitude'] = top_lon

# ==========================================
# 6. GERAR O MAPA COM FOLIUM E AJUSTAR ZOOM (FIT BOUNDS)
# ==========================================
mapa = folium.Map(tiles=None) # Removidas coordenadas iniciais e zoom estático

folium.TileLayer(
    tiles='http://mt0.google.com/vt/lyrs=s&hl=pt-BR&x={x}&y={y}&z={z}',
    attr='Google Satellite',
    name='Google Satellite',
    max_zoom=26,
).add_to(mapa)

coordenadas_gps = list(zip(gps_df['latitude'], gps_df['longitude']))
coordenadas_ratslam = list(zip(top_df['latitude'], top_df['longitude']))

fg_gps = folium.FeatureGroup(name='ground truth')
folium.PolyLine(locations=coordenadas_gps, color='blue', weight=3, opacity=0.8).add_to(fg_gps)
fg_gps.add_to(mapa)

fg_ratslam = folium.FeatureGroup(name='experience map')
folium.PolyLine(locations=coordenadas_ratslam, color='red', weight=3, opacity=0.9).add_to(fg_ratslam)
fg_ratslam.add_to(mapa)

# ==========================================
# 7. PLOTAR A DISTÂNCIA DE HAUSDORFF NO MAPA
# ==========================================
ponto_gps_max = coordenadas_gps[idx_max_gps]
ponto_ratslam_max = coordenadas_ratslam[idx_max_ratslam]

fg_hausdorff = folium.FeatureGroup(name='Hausdorff Max Distance')

folium.PolyLine(
    locations=[ponto_gps_max, ponto_ratslam_max],
    color='yellow', weight=4, opacity=1.0, dash_array='8',
    tooltip=f'Maior erro: {distancia_hausdorff:.2f}m'
).add_to(fg_hausdorff)

folium.CircleMarker(ponto_gps_max, radius=3, color='blue', fill=True, fill_opacity=1, popup='Ponto GPS').add_to(fg_hausdorff)
folium.CircleMarker(ponto_ratslam_max, radius=3, color='red', fill=True, fill_opacity=1, popup='Ponto RatSLAM').add_to(fg_hausdorff)
fg_hausdorff.add_to(mapa)

folium.LayerControl().add_to(mapa)

# CALCULAR BOUNDING BOX (Extremos para o zoom automático)
min_lat = min(gps_df['latitude'].min(), top_df['latitude'].min())
max_lat = max(gps_df['latitude'].max(), top_df['latitude'].max())
min_lon = min(gps_df['longitude'].min(), top_df['longitude'].min())
max_lon = max(gps_df['longitude'].max(), top_df['longitude'].max())

# Manda o mapa focar exatamente nestas coordenadas
mapa.fit_bounds([[min_lat, min_lon], [max_lat, max_lon]])

# ==========================================
# 8. LEGENDA PERSONALIZADA DO MAPA (Tamanho Automático)
# ==========================================
# Usando 'width: max-content' e 'height: auto' para encaixar perfeito, 
# e 'bottom: 30px' para colar um pouco mais no canto da tela.
legend_html = '''
<div style="
    position: fixed; 
    bottom: 30px; left: 30px; width: max-content; height: auto; 
    border:2px solid grey; z-index:9999; font-size:14px;
    background-color:white; opacity: 0.9; padding: 10px; border-radius: 5px;">
    <b style="display: block; margin-bottom: 5px;">Legenda</b>
    <div style="margin-bottom: 3px;"><i style="background:blue; width: 12px; height: 12px; display: inline-block; margin-right: 5px; vertical-align: middle;"></i> <span style="vertical-align: middle;">ground truth</span></div>
    <div style="margin-bottom: 3px;"><i style="background:red; width: 12px; height: 12px; display: inline-block; margin-right: 5px; vertical-align: middle;"></i> <span style="vertical-align: middle;">experience map</span></div>
    <div><i style="background:yellow; width: 12px; height: 12px; display: inline-block; margin-right: 5px; vertical-align: middle;"></i> <span style="vertical-align: middle;">Max Hausdorff</span></div>
</div>
'''
mapa.get_root().html.add_child(folium.Element(legend_html))

arquivo_saida = 'mapa_trajetorias_rotacionado.html'
mapa.save(arquivo_saida)
print(f"Mapa gerado com sucesso em: {arquivo_saida}")