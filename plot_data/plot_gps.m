clear;
clc;
close all;

%% Abrir bag
bag = ros2bagreader('~/Downloads/run_morato3');

%% Ler tópico GPS
gpsBag = select(bag,'Topic','/gps');
msgs = readMessages(gpsBag);

%% Extrair dados
n = length(msgs);

latitude  = zeros(n,1);
longitude = zeros(n,1);
altitude  = zeros(n,1);

for k = 1:n
    latitude(k)  = msgs{k}.latitude;
    longitude(k) = msgs{k}.longitude;
    altitude(k)  = msgs{k}.altitude;
end

%% Guardar dados originais
latitude_raw  = latitude;
longitude_raw = longitude;
altitude_raw  = altitude;

fprintf('\n====================================\n');
fprintf('TOTAL DE AMOSTRAS: %d\n', n);
fprintf('====================================\n');

%% Procurar amostras com latitude inválida
idx_lat = find(abs(latitude_raw) > 90);

fprintf('\n===== LATITUDES INVÁLIDAS =====\n');

if isempty(idx_lat)
    fprintf('Nenhuma latitude inválida encontrada.\n');
else
    fprintf('Amostras:\n');
    disp(idx_lat')

    for k = idx_lat'
        fprintf('Amostra %d -> Lat=%.6f Lon=%.6f Alt=%.2f\n', ...
            k, latitude_raw(k), longitude_raw(k), altitude_raw(k));
    end
end

%% Procurar amostras com longitude inválida
idx_lon = find(abs(longitude_raw) > 180);

fprintf('\n===== LONGITUDES INVÁLIDAS =====\n');

if isempty(idx_lon)
    fprintf('Nenhuma longitude inválida encontrada.\n');
else
    fprintf('Amostras:\n');
    disp(idx_lon')

    for k = idx_lon'
        fprintf('Amostra %d -> Lat=%.6f Lon=%.6f Alt=%.2f\n', ...
            k, latitude_raw(k), longitude_raw(k), altitude_raw(k));
    end
end

%% Procurar longitudes positivas (suspeitas)
idx_pos = find(longitude_raw > 0);

fprintf('\n===== LONGITUDES POSITIVAS =====\n');

if isempty(idx_pos)
    fprintf('Nenhuma longitude positiva encontrada.\n');
else
    fprintf('Amostras:\n');
    disp(idx_pos')

    for k = idx_pos'
        fprintf('Amostra %d -> Lat=%.6f Lon=%.6f Alt=%.2f\n', ...
            k, latitude_raw(k), longitude_raw(k), altitude_raw(k));
    end
end

%% Filtrar coordenadas válidas
idx_valid = ...
    latitude_raw  >= -90  & latitude_raw  <= 90 & ...
    longitude_raw >= -180 & longitude_raw <= 180;

latitude  = latitude_raw(idx_valid);
longitude = longitude_raw(idx_valid);
altitude  = altitude_raw(idx_valid);

%% Centro da trajetória
lat_ref = median(latitude);
lon_ref = median(longitude);

%% Filtrar pontos muito distantes
idx_region = ...
    abs(latitude  - lat_ref) < 0.01 & ...
    abs(longitude - lon_ref) < 0.01;

latitude  = latitude(idx_region);
longitude = longitude(idx_region);
altitude  = altitude(idx_region);

%% Estatísticas
fprintf('\n===== ESTATÍSTICAS =====\n');
fprintf('Latitude : %.6f -> %.6f\n', ...
        min(latitude), max(latitude));

fprintf('Longitude: %.6f -> %.6f\n', ...
        min(longitude), max(longitude));

%% Plot mapa
figure(1)

geoplot(latitude, longitude, ...
    'r-', 'LineWidth', 1.5);

hold on

geoscatter(latitude(1), longitude(1), ...
    80, 'g', 'filled');

geoscatter(latitude(end), longitude(end), ...
    80, 'r', 'filled');

geobasemap satellite

title('Trajetória GPS')

legend('Trajetória','Início','Fim')

%% Plot latitude
figure(2)
plot(latitude)
grid on
title('Latitude')

%% Plot longitude
figure(3)
plot(longitude)
grid on
title('Longitude')

%% Plot altitude
figure(4)
plot(altitude)
grid on
title('Altitude')