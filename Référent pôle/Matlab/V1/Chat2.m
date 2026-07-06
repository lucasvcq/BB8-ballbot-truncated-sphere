clear; clc;

% === Paramètres ===
m = 7;        % masse de la masse mobile (kg)
l = 0.3;        % distance centre -> masse (m)
g = 9.81;       % gravité (m/s^2)
I_r = 1/2*m*l^2;     % moment d'inertie de la roue
I_m = m * l^2;  % moment d'inertie de la masse
theta_c = deg2rad(30);  % angle cible (en rad)
Kp = 5;
Kd = 2;  % terme dérivé pour amortir
tau_ctrl = @(theta, theta_dot) Kp * (theta_c - theta) - Kd * theta_dot;

% === Conditions initiales ===
phi0 = 0;           % angle roue
phi_dot0 = 0;       % vitesse roue
theta0 = 0;         % angle masse (initialement en bas)
theta_dot0 = 0;     % vitesse masse
x0 = [phi0; phi_dot0; theta0; theta_dot0];

% === Temps de simulation ===
tspan = [0 5];

% === Simulation ===
[t, x] = ode45(@(t, x) system_dynamics(t, x, tau_ctrl), tspan, x0);

% === Affichage ===
phi = x(:,1); theta = x(:,3);

% Rayon de la roue
R = 0.15;  % exemple : 15 cm

% Calcul du déplacement
x_pos = R * phi;

% Affichage
figure;

subplot(3,1,1);
plot(t, rad2deg(phi), 'r', 'LineWidth', 2);
xlabel('Temps (s)');
ylabel('Angle roue (°)');
title('Rotation de la roue');
grid on;

subplot(3,1,2);
plot(t, rad2deg(theta), 'b', 'LineWidth', 2);
xlabel('Temps (s)');
ylabel('Angle masse (°)');
title('Position de la masse');
grid on;

subplot(3,1,3);
plot(t, x_pos, 'g', 'LineWidth', 2);
xlabel('Temps (s)');
ylabel('Position (m)');
title('Déplacement du robot');
grid on;

