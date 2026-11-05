window.HYDROWATCH_MQTT_CONFIG = {
  // Quando o painel e aberto pelo servidor local, usa automaticamente o
  // mesmo computador como broker WebSocket.
  brokerUrl: `ws://${window.location.hostname || "172.16.1.236"}:9001`,
  topicPrefix: "hydrowatch",
  username: "",
  password: "",
};
