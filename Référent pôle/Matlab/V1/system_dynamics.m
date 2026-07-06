function dxdt = system_dynamics(~, x, tau_ctrl)
    % x = [phi; phi_dot; theta; theta_dot]

    % Paramètres
    m = 0.5;
    l = 0.1;
    g = 9.81;
    I_r = 0.05;
    I_m = m * l^2;

    % États
    phi = x(1);
    phi_dot = x(2);
    theta = x(3);
    theta_dot = x(4);

    % Couple contrôlé avec PD
    tau = tau_ctrl(theta, theta_dot);

    % Équations du mouvement
    phi_ddot = (-tau - m * g * l * theta) / I_r;
    theta_ddot = (tau - m * g * l * sin(theta)) / I_m - (m * g * l * cos(theta) * theta) / I_m;

    dxdt = [phi_dot;
            phi_ddot;
            theta_dot;
            theta_ddot];
end
