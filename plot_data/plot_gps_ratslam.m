clear
clc
close all

%% Read CSV files

folder = "../../topological_logs/";

nodes = readtable(fullfile(folder,"topological_nodes.csv"));
gps   = readtable(fullfile(folder,"gps_log.csv"));

%% ============================================================
% Filter invalid GPS measurements
% =============================================================

idx_valid = ...
    gps.latitude  >= -90  & gps.latitude  <= 90 & ...
    gps.longitude >= -180 & gps.longitude <= 180;

gps = gps(idx_valid,:);

lat_ref = median(gps.latitude);
lon_ref = median(gps.longitude);

idx_region = ...
    abs(gps.latitude  - lat_ref) < 0.01 & ...
    abs(gps.longitude - lon_ref) < 0.01;

gps = gps(idx_region,:);

fprintf("GPS samples after filtering: %d\n", height(gps));

%% ============================================================
% Convert GPS to local coordinates (meters)
% =============================================================

lat0 = gps.latitude(1);
lon0 = gps.longitude(1);
h0   = gps.altitude(1);

[xGPS,yGPS,~] = geodetic2enu( ...
    gps.latitude,...
    gps.longitude,...
    gps.altitude,...
    lat0,lon0,h0,...
    wgs84Ellipsoid);

%% ============================================================
% Apply Procrustes Rotation and Scale
% =============================================================

theta = deg2rad(21.86);      % Rotation obtained with Procrustes
scale = 0.7108;              % Scale obtained with Procrustes

R = [ cos(theta) -sin(theta);
      sin(theta)  cos(theta)];

P = [nodes.x nodes.y]';

P = scale * R * P;

nodes.x = P(1,:)';
nodes.y = P(2,:)';

%% ============================================================
% Plot
% =============================================================

figure
hold on
grid on
axis equal

title("Topological Nodes vs GPS")
xlabel("X (m)")
ylabel("Y (m)")

%% ============================================================
% Draw Topological Nodes
% =============================================================

scatter(...
    nodes.x,...
    nodes.y,...
    40,...
    'b',...
    'filled');

%% ============================================================
% Draw GPS Trajectory
% =============================================================

plot(...
    xGPS,...
    yGPS,...
    'r-',...
    'LineWidth',2);

scatter(...
    xGPS(1),...
    yGPS(1),...
    100,...
    'g',...
    'filled');

scatter(...
    xGPS(end),...
    yGPS(end),...
    100,...
    'r',...
    'filled');

%% ============================================================
% Legend
% =============================================================

hNodes = scatter(nan,nan,40,'b','filled');
hGPS   = plot(nan,nan,'r-','LineWidth',2);
hStart = scatter(nan,nan,100,'g','filled');
hEnd   = scatter(nan,nan,100,'r','filled');

legend(...
    [hNodes hGPS hStart hEnd],...
    {'Topological Nodes',...
     'GPS Trajectory',...
     'GPS Start',...
     'GPS End'},...
     'Location','best');

axis equal