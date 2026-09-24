/*
 * Camada de apresentação estática do HydroWatch.
 * Integração futura: substitua mockData e as leituras locais por chamadas ao
 * cliente do Supabase. A navegação e a renderização visual podem ser mantidas.
 */

const mockData = {
  periods: {
    "24h": {
      average: "7,3 cm",
      trend: "+0,1 cm",
      line: "M14 104 C52 98 63 78 99 86 S151 111 183 93 S227 64 264 73 S317 83 346 65",
      area: "M14 104 C52 98 63 78 99 86 S151 111 183 93 S227 64 264 73 S317 83 346 65 L346 150 L14 150 Z",
      pointY: 65,
    },
    "7d": {
      average: "7,4 cm",
      trend: "+0,3 cm",
      line: "M14 91 C48 80 58 104 93 86 S143 68 180 76 S229 104 264 83 S314 53 346 61",
      area: "M14 91 C48 80 58 104 93 86 S143 68 180 76 S229 104 264 83 S314 53 346 61 L346 150 L14 150 Z",
      pointY: 61,
    },
    "30d": {
      average: "7,1 cm",
      trend: "−0,2 cm",
      line: "M14 55 C50 62 63 51 100 73 S150 96 182 84 S223 108 264 101 S310 75 346 83",
      area: "M14 55 C50 62 63 51 100 73 S150 96 182 84 S223 108 264 101 S310 75 346 83 L346 150 L14 150 Z",
      pointY: 83,
    },
  },
};

const screens = [...document.querySelectorAll("[data-screen]")];
const navItems = [...document.querySelectorAll(".nav-item")];

function navigateTo(target) {
  screens.forEach((screen) => {
    const active = screen.dataset.screen === target;
    screen.hidden = !active;
    screen.classList.toggle("is-active", active);
    if (active) screen.scrollTop = 0;
  });

  navItems.forEach((item) => {
    const active = item.dataset.target === target;
    item.classList.toggle("is-active", active);
    item.setAttribute("aria-current", active ? "page" : "false");
  });
}

navItems.forEach((item) => item.addEventListener("click", () => navigateTo(item.dataset.target)));
document.querySelectorAll("[data-go-to]").forEach((button) => {
  button.addEventListener("click", () => navigateTo(button.dataset.goTo));
});

const toast = document.querySelector("#toast");
let toastTimer;

function showToast(message) {
  toast.querySelector("span").textContent = message;
  toast.classList.add("is-visible");
  window.clearTimeout(toastTimer);
  toastTimer = window.setTimeout(() => toast.classList.remove("is-visible"), 2200);
}

document.querySelector("#refreshButton").addEventListener("click", (event) => {
  const button = event.currentTarget;
  button.classList.remove("is-spinning");
  void button.offsetWidth;
  button.classList.add("is-spinning");

  const now = new Date();
  const time = now.toLocaleTimeString("pt-BR", { hour: "2-digit", minute: "2-digit" });
  document.querySelector("#lastUpdate").textContent = `Atualizado às ${time}`;
  showToast("Dados atualizados");
});

document.querySelectorAll("[data-period]").forEach((button) => {
  button.addEventListener("click", () => {
    document.querySelectorAll("[data-period]").forEach((item) => item.classList.remove("is-selected"));
    button.classList.add("is-selected");

    const period = mockData.periods[button.dataset.period];
    document.querySelector("#averageLevel").textContent = period.average;
    document.querySelector(".trend").textContent = period.trend;
    document.querySelector("#chartLine").setAttribute("d", period.line);
    document.querySelector("#chartArea").setAttribute("d", period.area);
    document.querySelectorAll("#chartPoint circle").forEach((circle) => circle.setAttribute("cy", period.pointY));
  });
});

document.querySelector("#markAllRead").addEventListener("click", () => {
  document.querySelectorAll(".alert-item.is-unread").forEach((item) => {
    item.classList.remove("is-unread");
    item.classList.add("is-read");
  });
  document.querySelector("#alertCount").textContent = "0";
  document.querySelectorAll(".notification-dot, .nav-icon-wrap i").forEach((dot) => dot.hidden = true);
  showToast("Todos os alertas foram lidos");
});

document.querySelectorAll(".settings-list button").forEach((button) => {
  button.addEventListener("click", () => showToast("Disponível na próxima etapa"));
});

navigateTo("inicio");
