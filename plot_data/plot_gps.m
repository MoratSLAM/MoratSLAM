clear all; clc;

% Load the .mat files
gps_latlon= readtable('gps_latlon.csv', 'Delimiter', ',');

save('gps_latlon.mat', 'gps_latlon');

disp('Conversion completed!');
disp('Your data has been saved as a MATLAB "table".');

set(0, 'DefaultFigureWindowStyle', 'normal');
%% --- Maps --- %%
figure(1)
geoplot(gps_latlon.latitude, gps_latlon.longitude, 'r-', 'LineWidth', 1.5)
hold on
geoscatter(gps_latlon.latitude(1), gps_latlon.longitude(1), 50, 'go', 'filled')
geoscatter(gps_latlon.latitude(end), gps_latlon.longitude(end), 50, 'ro', 'filled')
geobasemap('satellite')
title('GPS route on map')

%% --- X Y--- %%
%figure(2);
%plot(gps_latlon.X, gps_latlon.Y, 'r');
%title('X and Y position');
%xlabel('X (m)');
%ylabel('Y (m)');
%grid on;

%% --- Lat --- %%
figure(3);
plot(gps_latlon.latitude, 'g')
title('Latitude');
xlabel('T');
ylabel('Lat');
grid on;

%% --- Lon --- %%
figure(4);
plot(gps_latlon.longitude, 'b');
title('Longitude');
xlabel('T');
ylabel('Lon');
grid on;