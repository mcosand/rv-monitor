'use strict';

const Hapi = require('hapi')

const db = require('./db')

// Create a server with a host and port
const server = new Hapi.Server();
server.connection({ 
  host: '0.0.0.0', 
  port: process.env.PORT || 5000 
});

server.register(require('vision'), (err) => {
  server.views({
    engines: {
      html: require('handlebars')
    },
    relativeTo: __dirname,
    path: 'templates'
  });
});

// Add the route
server.route({
  method: 'GET',
  path:'/',
  handler: function (request, reply) {
    return reply.view('index');
  }
});

function parseData(d) {
  var results = [];
  results.push(d[0] == '_' ? null : d[0]*1);
  var ds = d.substr(1).split(',')
  for (var i=0;i<ds.length;i++) results.push(ds[i]*1)
  return results;
}

server.route({
  method: 'GET',
  path: '/data',
  handler: function(request, reply) {
    return reply(db('packets').where('type', 's').where('published', '>', 1492754661614).orderBy('published').map(d => [d.published, parseData(d.data)]))
  }
})

server.route({
  method: 'GET',
  path: '/last',
  handler: function(request, reply) {
    return reply(db('packets').orderBy('published', 'desc').limit(5))
  }
})

server.route({
  method: 'POST',
  path:'/particle',
  handler: function(request, reply) {
    const d = request.payload
    var row = {
      type: d.event,
      data: d.data,
      raw: JSON.stringify(d),
      published: new Date(d.published_at)
    }
    console.log(row.published)
    return db.insert(row).into("packets").then(function (id) {
      console.log(id[0], d.event, d.data);
    })
    .then(() => reply({}).code(200))
    .catch((err) => {
      console.log(err)
      return reply(err + '').code(500)
    })
  }
})

server.start((err) => {
  if (err) {
    throw err;
  }
  console.log('Server running at:', server.info.uri);
})