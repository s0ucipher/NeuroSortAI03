const { methodNotAllowed, webUnsupported } = require('./_organizer')

module.exports = function handler(req, res) {
  if (req.method !== 'POST') {
    methodNotAllowed(res)
    return
  }

  webUnsupported(res, 'Saving locally is not supported in web mode. Files will be zipped and downloaded through your browser.')
}
