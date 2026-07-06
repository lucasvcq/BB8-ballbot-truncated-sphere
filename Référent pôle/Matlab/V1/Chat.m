% Paramètres
m = 0.5;        % Masse de la masse mobile (kg)
l = 0.1;        % Longueur (distance entre le centre de la roue et la masse) (m)
g = 9.81;       % Gravité (m/s^2)
I_r = 0.05;     % Moment d'inertie de la roue (kg.m^2)
I_m = m * l^2;  % Moment d'inertie de la masse (kg.m^2)

% Angle cible (theta_c)
theta_c = pi / 6;  % Exemple : 30 degrés (ou pi/6 radians)

% Calcul des termes linéarisés
A_1 = - m * g * l * cos(theta_c) / I_r; % Coefficient de la masse dans l'équation de la roue
A_2 = m * g * l * sin(theta_c) / I_m;   % Coefficient de la masse dans l'équation de la masse

% Fonction dynamique du système
function dxdt = robot_dynamics(t, x, tau)
    % État : x = [phi, phi_dot, theta, theta_dot]
    % tau est l'entrée (couple moteur)

    phi = x(1);
    phi_dot = x(2);
    theta = x(3);
    theta_dot = x(4);

    % Paramètres
    m = 0.5;        % Masse de la masse mobile (kg)
    l = 0.1;        % Longueur (m)
    g = 9.81;       % Gravité (m/s^2)
    I_r = 0.05;     % Moment d'inertie de la roue (kg.m^2)
    I_m = m * l^2;  % Moment d'inertie de la masse (kg.m^2)
    theta_c = pi / 6;  % Angle cible (rad)

    % Dynamique du système linéarisé
    phi_ddot = (-tau - m * g * l * cos(theta_c) * theta) / I_r;
    theta_ddot = (tau - m * g * l * sin(theta_c)) / I_m - (m * g * l * cos(theta_c) * theta) / I_m;

    % Equation d'état
    dxdt = [phi_dot; phi_ddot; theta_dot; theta_ddot];
end

% Conditions initiales : [phi, phi_dot, theta, theta_dot]
x0 = [0; 0; pi / 6; 0];  % Par exemple, phi=0, theta=30° (pi/6), et vitesses nulles

% Temps de simulation
tspan = [0 10];  % Simuler pendant 10 secondes

% Entrée du système : couple moteur (par exemple, une impulsion ou une fonction de commande)
tau = 0.1;  % Couple constant pour l'exemple

% Résolution de l'ODE
[t, x] = ode45(@(t, x) robot_dynamics(t, x, tau), tspan, x0);

% Extraire les résultats
phi = x(:, 1);  % Angle de la roue
theta = x(:, 3);  % Angle de la masse

% Visualisation des résultats
figure;
subplot(2, 1, 1);
plot(t, phi, 'r', 'LineWidth', 2);
title('Angle de la roue (phi) vs Temps');
xlabel('Temps (s)');
ylabel('Angle de la roue (rad)');

subplot(2, 1, 2);
plot(t, theta, 'b', 'LineWidth', 2);
title('Angle de la masse (theta) vs Temps');
xlabel('Temps (s)');
ylabel('Angle de la masse (rad)');
