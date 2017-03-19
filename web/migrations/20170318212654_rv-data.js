
exports.up = function(knex, Promise) {
  return Promise.all([
    knex.schema.createTable('packets', function(table) {
      table.increments('id').primary();
      table.string('type');
      table.string('data');
      table.string('raw');
      table.dateTime('published');
    })
  ])
};

exports.down = function(knex, Promise) {
  return Promise.all([
    knex.schema.dropTable('packets')
  ])
};
