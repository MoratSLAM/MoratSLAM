clear;
clc;
close all;

%% =======================================================
%% 1. ABRIR BAGS E LER TÓPICOS
%% =======================================================
fprintf('Lendo bags...\n');

% Bag do GPS
bag_gps = ros2bagreader('~/Documents/tcc/run_morato3.1');
gpsBag = select(bag_gps, 'Topic', '/gps');
msgs_gps = readMessages(gpsBag);

% Bag do RatSLAM
bag_slam = ros2bagreader('~/Documents/tcc/output_run_morato3.1');
slamBag = select(bag_slam, 'Topic', '/irat_red/ExperienceMap/RobotPose');
msgs_slam = readMessages(slamBag);

%% =======================================================
%% 2. EXTRAIR DADOS DO GPS (COM TIMESTAMP)
%% =======================================================
n_gps = length(msgs_gps);

latitude_raw  = zeros(n_gps,1);
longitude_raw = zeros(n_gps,1);
altitude_raw  = zeros(n_gps,1);
time_gps_raw  = zeros(n_gps,1); % Array para os timestamps do GPS

for k = 1:n_gps
    latitude_raw(k)  = msgs_gps{k}.latitude;
    longitude_raw(k) = msgs_gps{k}.longitude;
    altitude_raw(k)  = msgs_gps{k}.altitude;
    
    % Extraindo timestamp do cabeçalho da mensagem (segundos + nanosegundos)
    sec = double(msgs_gps{k}.header.stamp.sec);
    nanosec = double(msgs_gps{k}.header.stamp.nanosec);
    time_gps_raw(k) = sec + (nanosec * 1e-9);
end

% Filtrar coordenadas válidas (ignorando lixos de GPS)
idx_valid = latitude_raw >= -90 & latitude_raw <= 90 & ...
            longitude_raw >= -180 & longitude_raw <= 180;

latitude  = latitude_raw(idx_valid);
longitude = longitude_raw(idx_valid);
altitude  = altitude_raw(idx_valid);
time_gps  = time_gps_raw(idx_valid);

% Como pode haver timestamps duplicados, vamos pegar apenas os únicos
% (necessário para a função interp1 funcionar corretamente depois)
[time_gps, idx_unq] = unique(time_gps);
latitude = latitude(idx_unq);
longitude = longitude(idx_unq);

%% =======================================================
%% 3. CONVERTER GPS (LAT/LON) PARA CARTESIANO LOCAL (X, Y)
%% =======================================================
% Usaremos o primeiro ponto válido de GPS como a origem (0,0)
lat0 = latitude(1);
lon0 = longitude(1);

% Raio aproximado da Terra em metros
R_earth = 6378137.0;

% Conversão simplificada Equiretangular (boa para curtas distâncias)
% x = (lon - lon0) * cos(lat0) * R_earth
% y = (lat - lat0) * R_earth
gps_x = (longitude - lon0) .* (pi/180) .* R_earth .* cos(lat0 * pi/180);
gps_y = (latitude - lat0) .* (pi/180) .* R_earth;


%% =======================================================
%% 4. EXTRAIR DADOS DO RATSLAM (COM TIMESTAMP)
%% =======================================================
n_slam = length(msgs_slam);

slam_x    = zeros(n_slam, 1);
slam_y    = zeros(n_slam, 1);
time_slam = zeros(n_slam, 1);

for k = 1:n_slam
    % RatSLAM geralmente exporta no RobotPose a posição no eixo X e Y
    slam_x(k) = msgs_slam{k}.pose.position.x;
    slam_y(k) = msgs_slam{k}.pose.position.y;
    
    % Timestamp da mensagem do SLAM
    sec = double(msgs_slam{k}.header.stamp.sec);
    nanosec = double(msgs_slam{k}.header.stamp.nanosec);
    time_slam(k) = sec + (nanosec * 1e-9);
end

%% =======================================================
%% 5. SINCRONIZAÇÃO VIA INTERPOLAÇÃO TEMPORAL
%% =======================================================
% O SLAM costuma ter uma frequência maior que o GPS.
% Vamos interpolar os dados cartesianos do GPS para baterem exatamente 
% nos instantes de tempo (timestamps) das mensagens do RatSLAM.

gps_x_interp = interp1(time_gps, gps_x, time_slam, 'linear', 'extrap');
gps_y_interp = interp1(time_gps, gps_y, time_slam, 'linear', 'extrap');

%% =======================================================
%% 6. PLOTAGEM PARA COMPARAÇÃO
%% =======================================================

% PLOT 1: GPS Bruto no Mapa (Geoplot) - Apenas para referência
figure('Name', 'Trajetória GPS Global', 'NumberTitle', 'off');
geoplot(latitude, longitude, 'r-', 'LineWidth', 1.5); hold on;
geoscatter(latitude(1), longitude(1), 80, 'g', 'filled');
geoscatter(latitude(end), longitude(end), 80, 'r', 'filled');
geobasemap satellite;
title('Trajetória GPS (Ground Truth)');
legend('Trajetória', 'Início', 'Fim');

% PLOT 2: Comparação Cartesiana (SLAM vs GPS Sincronizado)
figure('Name', 'Comparação SLAM vs GPS', 'NumberTitle', 'off');
plot(gps_x_interp, gps_y_interp, 'r-', 'LineWidth', 2); hold on;
plot(slam_x, slam_y, 'b-', 'LineWidth', 1.5);
scatter(gps_x_interp(1), gps_y_interp(1), 80, 'g', 'filled'); % Ponto inicial
grid on; axis equal;

title('Comparação Sincronizada: RatSLAM vs GPS (Ground Truth)');
xlabel('Distância X (metros)');
ylabel('Distância Y (metros)');
legend('GPS Interpolado', 'OpenRatSLAM', 'Origem (Início)');