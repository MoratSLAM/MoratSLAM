clear all; clc; close all;

%% Carrega o CSV
data = readtable('odom_velocity.csv');

% Se quiser salvar como .mat
save('odom_velocity.mat', 'data');

disp('Dados carregados com sucesso!');

%% Extrai variáveis
t = data.time;

lin_x = data.lin_x;
lin_y = data.lin_y;
lin_z = data.lin_z;

ang_x = data.ang_x;
ang_y = data.ang_y;
ang_z = data.ang_z;

%% ===== VELOCIDADE LINEAR =====
figure;
plot(t, lin_x, 'LineWidth', 1.5);
grid on;
xlabel('Tempo [s]');
ylabel('Velocidade Linear X [m/s]');
title('Velocidade Linear - Eixo X');

figure;
plot(t, lin_y, 'LineWidth', 1.5);
grid on;
xlabel('Tempo [s]');
ylabel('Velocidade Linear Y [m/s]');
title('Velocidade Linear - Eixo Y');

figure;
plot(t, lin_z, 'LineWidth', 1.5);
grid on;
xlabel('Tempo [s]');
ylabel('Velocidade Linear Z [m/s]');
title('Velocidade Linear - Eixo Z');

%% ===== VELOCIDADE ANGULAR =====
figure;
plot(t, ang_x, 'LineWidth', 1.5);
grid on;
xlabel('Tempo [s]');
ylabel('Velocidade Angular X [rad/s]');
title('Velocidade Angular - Eixo X');

figure;
plot(t, ang_y, 'LineWidth', 1.5);
grid on;
xlabel('Tempo [s]');
ylabel('Velocidade Angular Y [rad/s]');
title('Velocidade Angular - Eixo Y');

figure;
plot(t, ang_z, 'LineWidth', 1.5);
grid on;
xlabel('Tempo [s]');
ylabel('Velocidade Angular Z [rad/s]');
title('Velocidade Angular - Eixo Z');
