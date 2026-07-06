// Attendre que l'animation se termine avant d'afficher la page principale
document.addEventListener("DOMContentLoaded", () => {
    const intro = document.getElementById("intro");
    const main = document.getElementById("main");
  
// Après 2.5 secondes (durée totale des animations)
    setTimeout(() => {
        intro.style.display = "none"; // Cache l'intro
        main.style.display = "block"; // Affiche la page principale
    }, 2500);
});