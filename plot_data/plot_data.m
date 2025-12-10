clear all; clc;

% Load the .mat files
serial_data= readtable('serial_data.csv', 'Delimiter', ';');

save('serial_data.mat', 'serial_data');

disp('Conversion completed!');
disp('Your data has been saved as a MATLAB "table".');

set(0, 'DefaultFigureWindowStyle', 'normal');
%% --- Maps --- %%
figure(1)
geoplot(serial_data.Lat, serial_data.Lon, 'r-', 'LineWidth', 1.5)
hold on
geoscatter(serial_data.Lat(1), serial_data.Lon(1), 50, 'go', 'filled')
geoscatter(serial_data.Lat(end), serial_data.Lon(end), 50, 'ro', 'filled')
geobasemap('satellite')
title('GPS route on map')

%% --- X Y--- %%
figure(2);
plot(serial_data.X, serial_data.Y, 'r');
title('X and Y position');
xlabel('X (m)');
ylabel('Y (m)');
grid on;

%% --- Lat --- %%
figure(3);
plot(serial_data.Lat, 'g')
title('Latitude');
xlabel('T');
ylabel('Lat');
grid on;

%% --- Lon --- %%
figure(4);
plot(serial_data.Lon, 'b');
title('Longitude');
xlabel('T');
ylabel('Lon');
grid on;