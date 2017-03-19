'use strict';

const Hapi = require('hapi')

const db = require('./db')

// Create a server with a host and port
const server = new Hapi.Server();
server.connection({ 
  host: '0.0.0.0', 
  port: process.env.PORT || 5000 
});

// Add the route
server.route({
  method: 'GET',
  path:'/',
  handler: function (request, reply) {
    return reply('hello world');
  }
});
server.route({
  method: 'POST',
  path:'/particle',
  handler: function(request, reply) {
    const d = request.payload
    var row = {
      type: d.name,
      data: d.data,
      raw: JSON.stringify(d),
      published: new Date(d.published)
    }
    return db.insert(row).into("packets").then(function (id) {
      console.log(id[0], d.name, d.data);
    }).then(() => reply({}).code(200));
  }
})

server.start((err) => {
  if (err) {
    throw err;
  }
  console.log('Server running at:', server.info.uri);
})