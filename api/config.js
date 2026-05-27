const { methodNotAllowed } = require('./_organizer')

module.exports = function handler(req, res) {
  if (req.method !== 'GET') {
    methodNotAllowed(res)
    return
  }

  res.status(200).json({ envMode: 'web' })
}
