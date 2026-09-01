const express = require('express');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = 3000;

app.use(cors());

// Статическая раздача папки public (включая public/firmware)
app.use(express.static(path.join(__dirname, 'public')));

// Логирование запросов от ESP32
app.use((req, res, next) => {
  console.log(`[${new Date().toLocaleTimeString()}] ${req.method} ${req.url} - IP: ${req.ip}`);
  next();
});

// Роут проверки работы сервера
app.get('/', (req, res) => {
  res.send('TV Lift Backend Service is Running');
});

app.listen(PORT, '0.0.0.0', () => {
  console.log(`=================================`);
  console.log(`OTA Server listening on port ${PORT}`);
  console.log(`Manifest URL: http://192.168.88.33:${PORT}/firmware/version.json`);
  console.log(`=================================`);
});