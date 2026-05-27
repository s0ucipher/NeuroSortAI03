const { methodNotAllowed, organizeMetadata, parseJsonBody } = require('./_organizer')

module.exports = function handler(req, res) {
  if (req.method !== 'POST') {
    methodNotAllowed(res)
    return
  }

  try {
    const body = parseJsonBody(req)
    if (!Array.isArray(body.files_metadata)) {
      res.status(400).json({ error: 'Local paths are not supported in web mode. Please upload files directly.' })
      return
    }

    res.status(200).json(organizeMetadata(body.files_metadata, body.sortBy || 'name'))
  } catch (error) {
    res.status(400).json({ error: 'Invalid JSON request body.' })
  }
}
