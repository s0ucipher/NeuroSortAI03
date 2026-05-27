const { methodNotAllowed, webUnsupported } = require('./_organizer')

module.exports = function handler(req, res) {
  if (req.method !== 'GET') {
    methodNotAllowed(res)
    return
  }

  webUnsupported(res, 'Folder picker is not supported in web mode. Please drag and drop or upload folders directly.')
}
