const { methodNotAllowed, webUnsupported } = require('./_organizer')

module.exports = function handler(req, res) {
  if (req.method !== 'GET') {
    methodNotAllowed(res)
    return
  }

  webUnsupported(res, 'File picker is not supported in web mode. Please drag and drop or upload files directly.')
}
