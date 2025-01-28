const express = require('express');
let app = express();

// add public dir
app.get("/", (req, res) => {
    res.sendFile()
});