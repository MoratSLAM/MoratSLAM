clear all; clc;

% Load the .mat files
% 1. Use 'readtable' and specify the delimiter ';'
serial_data= readtable('serial_data.csv', 'Delimiter', ';');

% 2. Save the variable 'data' (which is a table) to a .mat file
save('serial_data.mat', 'serial_data');

disp('Conversion completed!');
disp('Your data has been saved as a MATLAB "table".');

%% --- X Y--- %%
figure(1);
plot(serial_data.X, serial_data.Y, 'r');
title('X and Y position');
xlabel('X (m)');
ylabel('Y (m)');
grid on;

%% --- Lat --- %%
figure(2);
plot(serial_data.Lat, 'g')
title('Latitude');
xlabel('T');
ylabel('Lat');
grid on;

%% --- Lon --- %%
figure(3);
plot(serial_data.Lon, 'b');
title('Longitude');
xlabel('T');
ylabel('Lon');
grid on;