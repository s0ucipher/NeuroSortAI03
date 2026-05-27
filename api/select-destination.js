const { methodNotAllowed, webUnsupported } = require('./_organizer')

module.exports = function handler(req, res) {
  if (req.method !== 'GET') {
    methodNotAllowed(res)
    return
  }

  webUnsupported(res, 'Destination picker is not supported in web mode.')
}
