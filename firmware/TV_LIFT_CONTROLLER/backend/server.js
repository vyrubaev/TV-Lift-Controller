const express = require('express');
const cors = require('cors');
const path = require('path');

const app = express();
const PORT = 3000;

app.use(cors());

// 1. Логирование ВСЕХ запросов (помещено ВЫШЕ статики)
app.use((req, res, next) => {
  console.log(`[${new Date().toLocaleTimeString()}] ${req.method} ${req.url} - IP: ${req.ip}`);
  next();
});

// 2. Статическая раздача папки public (файлы будут доступны по адресам /firmware/version.json и т.д.)
app.use(express.static(path.join(__dirname, 'public')));

// 3. Роут проверки работы сервера
app.get('/', (req, res) => {
  res.send('TV Lift Backend Service is Running');
});

app.listen(PORT, '0.0.0.0', () => {
  console.log(`=================================`);
  console.log(`OTA Server listening on port ${PORT}`);
  console.log(`Manifest URL: http://192.168.88.33:${PORT}/firmware/version.json`);
  console.log(`=================================`);
});
