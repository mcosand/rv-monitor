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

function storeData(d, reply) {
  var row = {
    type: d.event,
    data: d.data,
    raw: JSON.stringify(d),
    published: new Date(d.published)
  }
  console.log(row)
  return db.insert(row).into("packets").then(function (id) {
    console.log(id[0], d.event, d.data);
  })
  .then(() => reply({}).code(200))
  .catch((err) => {
    console.log(err)
    return reply(err + '').code(500)
  })
}

server.route({
  method: 'POST',
  path:'/particle',
  handler: function(request, reply) {
    return storeData(request.payload, reply)
  }
})
server.route({
  method: 'PUT',
  path: '/particle',
  handler: function(request, reply) {
    return storeData(request.payload, reply);
  }
})
server.route({
  method: 'GET',
  path:'/particle',
  handler: function(request, reply) {
    return storeData(request.url.query, reply)
  }
})

server.start((err) => {
  if (err) {
    throw err;
  }
  console.log('Server running at:', server.info.uri);
})