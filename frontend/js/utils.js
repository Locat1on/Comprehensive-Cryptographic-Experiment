function copyText(t) {
  navigator.clipboard?.writeText(t).catch(() => {});
}

function timestamp() {
  return new Date().toLocaleTimeString();
}
