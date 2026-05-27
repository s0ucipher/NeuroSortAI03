const CATEGORIES = {
  STUDY: 'Study',
  IMAGES: 'Images',
  DOCUMENTS: 'Documents',
  VIDEOS: 'Videos',
  OTHERS: 'Others',
}

const FOLDERS = {
  [CATEGORIES.STUDY]: 'Study Hub',
  [CATEGORIES.IMAGES]: 'Images',
  [CATEGORIES.DOCUMENTS]: 'Documents',
  [CATEGORIES.VIDEOS]: 'Videos',
  [CATEGORIES.OTHERS]: 'Others',
}

const IMAGE_EXTS = ['.jpg', '.jpeg', '.png', '.gif', '.webp', '.heic', '.svg']
const DOC_EXTS = ['.pdf', '.docx', '.doc', '.txt', '.pptx', '.xlsx', '.csv']
const VIDEO_EXTS = ['.mp4', '.mkv', '.mov', '.avi', '.webm']

const VERY_HIGH_KEYWORDS = [
  'passport', 'aadhar', 'aadhaar', 'pan card', 'pancard', 'driving licence',
  'driving license', 'driverlicense', 'voter id', 'voterid', 'voter card',
  'national id', 'ssn', 'social security', 'ration card', 'visa', 'work permit',
  'residence permit', 'marksheet', 'mark sheet', 'markcard', 'grade card',
  'result', 'scorecard', 'score card', 'admit card', 'admitcard', 'hall ticket',
  'hallticket', 'registration', 'enrollment', 'enrolment', 'roll number',
  'degree certificate', 'provisional certificate', 'migration certificate',
  'character certificate', 'passing certificate', 'passing cert', 'bonafide',
  'transfer certificate', 'tc ', 'tax return', 'itr', 'form16', 'form 16',
  'gst', 'tds certificate', 'income tax', 'w2', 'w-2', '1099', 'tax invoice',
  'birth certificate', 'death certificate', 'marriage certificate', 'divorce',
  'adoption', 'property deed', 'land deed', 'sale deed', 'power of attorney',
  'affidavit', 'will ', 'agreement', 'legal notice', 'court order', 'fir',
  'police report', 'summons',
]

const HIGH_KEYWORDS = [
  'bank statement', 'bank account', 'account statement', 'passbook', 'cheque',
  'check', 'demand draft', 'invoice', 'receipt', 'billing', 'bill ', 'salary',
  'payslip', 'pay slip', 'payroll', 'credit card', 'loan', 'mortgage',
  'emi statement', 'insurance policy', 'insurance', 'premium', 'mutual fund',
  'portfolio', 'investment', 'stock', 'demat', 'trading', 'balance sheet',
  'profit loss', 'financial statement', 'medical', 'prescription', 'medicine',
  'health report', 'lab report', 'pathology', 'diagnosis', 'discharge summary',
  'hospital', 'vaccination', 'vaccine', 'immunization', 'blood report', 'xray',
  'x-ray', 'mri', 'scan report', 'resume', 'cv ', 'curriculum vitae',
  'offer letter', 'appointment letter', 'experience letter', 'relieving letter',
  'joining letter', 'noc', 'no objection', 'recommendation', 'certificate',
  'certification', 'diploma', 'transcript', 'academic record', 'exam result',
  'examination', 'final exam', 'semester', 'annual result',
]

const MEDIUM_KEYWORDS = [
  'notes', 'lecture', 'handout', 'syllabus', 'textbook', 'chapter',
  'assignment', 'homework', 'thesis', 'dissertation', 'research', 'paper',
  'study', 'revision', 'summary', 'project', 'report', 'presentation',
  'proposal', 'draft', 'plan', 'roadmap', 'specification', 'design',
  'architecture', 'documentation', 'minutes', 'agenda', 'meeting notes',
  'progress', 'update', 'status report', 'contract draft', 'quotation',
  'estimate', 'portfolio', 'creative', 'artwork', 'design work', 'script',
  'manuscript', 'storyboard',
]

function parseJsonBody(req) {
  if (!req.body) return {}
  if (typeof req.body === 'object') return req.body
  return JSON.parse(req.body)
}

function methodNotAllowed(res) {
  res.status(405).json({ error: 'Method Not Allowed' })
}

function webUnsupported(res, message) {
  res.status(400).json({ error: message })
}

function hasExtension(name, extensions) {
  return extensions.some((ext) => name.endsWith(ext))
}

function classifyFile(name) {
  const lowerName = name.toLowerCase()

  if (lowerName.includes('notes') || lowerName.includes('assignment')) {
    return CATEGORIES.STUDY
  }
  if (hasExtension(lowerName, ['.jpg', '.png'])) return CATEGORIES.IMAGES
  if (hasExtension(lowerName, ['.pdf', '.docx'])) return CATEGORIES.DOCUMENTS
  if (hasExtension(lowerName, ['.mp4', '.mkv'])) return CATEGORIES.VIDEOS
  if (hasExtension(lowerName, IMAGE_EXTS)) return CATEGORIES.IMAGES
  if (hasExtension(lowerName, DOC_EXTS)) return CATEGORIES.DOCUMENTS
  if (hasExtension(lowerName, VIDEO_EXTS)) return CATEGORIES.VIDEOS

  return CATEGORIES.OTHERS
}

function fileType(name) {
  const lastDot = name.lastIndexOf('.')
  return lastDot === -1 ? 'no extension' : name.slice(lastDot).toLowerCase()
}

function scoreFile(name, sizeBytes) {
  const lowerName = name.toLowerCase()
  let range = [0, 30]

  if (VERY_HIGH_KEYWORDS.some((keyword) => lowerName.includes(keyword))) {
    range = [90, 100]
  } else if (HIGH_KEYWORDS.some((keyword) => lowerName.includes(keyword))) {
    range = [80, 90]
  } else if (MEDIUM_KEYWORDS.some((keyword) => lowerName.includes(keyword))) {
    range = [50, 80]
  } else if (classifyFile(name) === CATEGORIES.DOCUMENTS) {
    range = [30, 50]
  }

  let hash = 0
  for (let i = 0; i < lowerName.length; i += 1) {
    hash = (hash * 31 + lowerName.charCodeAt(i)) >>> 0
  }
  hash = (hash + (Number(sizeBytes) >>> 0)) >>> 0

  const spread = Math.max(1, range[1] - range[0])
  return Math.min(1, Math.max(0, (range[0] + (hash % spread)) / 100))
}

function priorityForScore(score) {
  if (score <= 0.3) {
    return {
      priority: 'Very Low',
      reason: 's0ucipher rated this Very Low (0-30%): generic media or unclassified file with no sensitive keywords detected.',
    }
  }
  if (score <= 0.5) {
    return {
      priority: 'Low',
      reason: 's0ucipher rated this Low (30-50%): standard document without specific sensitive identifiers.',
    }
  }
  if (score <= 0.8) {
    return {
      priority: 'Medium',
      reason: 's0ucipher rated this Medium (50-80%): academic notes, project work, or general study material detected.',
    }
  }
  if (score <= 0.9) {
    return {
      priority: 'High',
      reason: 's0ucipher rated this High (80-90%): financial record, medical data, professional certificate, or official academic document detected.',
    }
  }
  return {
    priority: 'Very High',
    reason: 's0ucipher rated this Very High (90-100%): critical government ID, marksheet, result, registration, admit card, legal deed, or tax document detected.',
  }
}

function suggestedName(name, category, priority) {
  const lastDot = name.lastIndexOf('.')
  const base = (lastDot === -1 ? name : name.slice(0, lastDot)).replaceAll(' ', '_')
  const ext = lastDot === -1 ? '' : name.slice(lastDot)

  if (priority === 'High') return `Important_${base}${ext}`
  if (category === CATEGORIES.STUDY) return `Study_${base}${ext}`
  return `${category}_${base}${ext}`
}

function buildRecord(meta, duplicateCount) {
  const name = String(meta.name || '')
  const source = String(meta.path || name)
  const sizeBytes = Number(meta.size_bytes || meta.size || 0)
  const modifiedAtMs = Number(meta.lastModified || Date.now())
  const category = classifyFile(name)
  const confidence = scoreFile(name, sizeBytes)
  const { priority, reason } = priorityForScore(confidence)
  const tags = []
  const lowerName = name.toLowerCase()

  if (category === CATEGORIES.STUDY) tags.push('academic')
  if (priority === 'High' || priority === 'Very High') tags.push('exam-ready')
  if (duplicateCount > 1) tags.push('duplicate-candidate')
  if (modifiedAtMs < Date.now() - 180 * 24 * 60 * 60 * 1000) tags.push('cleanup-candidate')
  if (lowerName.includes('project') || lowerName.includes('report')) tags.push('project-work')
  if (category === CATEGORIES.IMAGES || category === CATEGORIES.VIDEOS) tags.push('media')
  if (tags.length === 0) tags.push('standard')

  return {
    name,
    source,
    type: fileType(name),
    category,
    priority,
    folder: FOLDERS[category],
    size_bytes: sizeBytes,
    modified_at: new Date(modifiedAtMs).toISOString().slice(0, 19),
    smart_tags: tags,
    ai_reason: reason,
    confidence: Number(confidence.toFixed(2)),
    suggested_name: suggestedName(name, category, priority),
  }
}

function sortRecords(records, sortBy) {
  return [...records].sort((a, b) => {
    if (sortBy === 'type') {
      const typeCompare = a.type.localeCompare(b.type, undefined, { sensitivity: 'accent' })
      if (typeCompare !== 0) return typeCompare
    }

    const nameCompare = a.name.localeCompare(b.name, undefined, { sensitivity: 'accent' })
    if (nameCompare !== 0) return nameCompare
    return a.source.localeCompare(b.source, undefined, { sensitivity: 'accent' })
  })
}

function groupBy(records, key) {
  return records.reduce((grouped, record) => {
    const value = record[key]
    if (!grouped[value]) grouped[value] = []
    grouped[value].push(record)
    return grouped
  }, {})
}

function organizeMetadata(filesMetadata, sortBy = 'name') {
  const duplicateCounts = filesMetadata.map((meta) => {
    const name = String(meta.name || '').toLowerCase()
    return filesMetadata.filter((candidate) => String(candidate.name || '').toLowerCase() === name).length
  })

  const before = sortRecords(
    filesMetadata.map((meta, index) => buildRecord(meta, duplicateCounts[index])),
    sortBy,
  )

  const duplicateGroups = Object.entries(groupBy(before, 'name'))
    .filter(([, files]) => files.length > 1)
    .map(([name, files]) => ({ name, count: files.length, files }))

  const studyFiles = before.filter((record) => record.category === CATEGORIES.STUDY)
  const importantFiles = before.filter((record) => record.priority === 'High' || record.priority === 'Very High')
  const cleanupSuggestions = before.flatMap((record) => {
    const suggestions = []
    if (record.smart_tags.includes('duplicate-candidate')) {
      suggestions.push({
        file: record.name,
        source: record.source,
        reason: 'Possible duplicate name across selected sources. Review before deleting.',
      })
    }
    if (record.smart_tags.includes('cleanup-candidate')) {
      suggestions.push({
        file: record.name,
        source: record.source,
        reason: 'Old file. Consider archiving or cleaning it up.',
      })
    }
    return suggestions
  })

  const categories = groupBy(before, 'category')
  const afterStructure = groupBy(before, 'folder')
  const categoryCounts = Object.fromEntries(Object.values(CATEGORIES).map((category) => [category, categories[category]?.length || 0]))
  const strongestCategory = Object.entries(categoryCounts).sort((a, b) => b[1] - a[1])[0]?.[0] || 'Mixed'
  const recommendedActions = []

  if (studyFiles.length > 0) recommendedActions.push(`Create a Study Hub with ${studyFiles.length} academic file(s).`)
  if (importantFiles.length > 0) recommendedActions.push(`Highlight ${importantFiles.length} exam/final file(s) as high priority.`)
  if (duplicateGroups.length > 0) recommendedActions.push(`Review ${duplicateGroups.length} duplicate name group(s) before deleting anything.`)
  if (cleanupSuggestions.length > 0) recommendedActions.push(`Archive or review ${cleanupSuggestions.length} old or duplicate file(s).`)
  if (recommendedActions.length === 0) {
    recommendedActions.push('No urgent cleanup found. Organize by category and keep preview mode for safety.')
  }

  return {
    path: 'NeuroSort Organized',
    destination_path: 'NeuroSort Organized',
    sort_by: sortBy,
    applied_changes: false,
    sources: [],
    before,
    categories,
    after_structure: afterStructure,
    duplicates: duplicateGroups,
    study_files: studyFiles,
    important_files: importantFiles,
    priorities: {
      High: before.filter((record) => record.priority === 'High'),
      Medium: before.filter((record) => record.priority === 'Medium'),
      Low: before.filter((record) => record.priority === 'Low'),
    },
    cleanup_suggestions: cleanupSuggestions,
    moved_files: [],
    dashboard: {
      total_files: before.length,
      source_count: 0,
      total_categories: Object.values(categoryCounts).filter((count) => count > 0).length,
      important_files: importantFiles.length,
      study_files: studyFiles.length,
      duplicate_groups: duplicateGroups.length,
      cleanup_items: cleanupSuggestions.length,
    },
    assistant: {
      name: 's0ucipher',
      summary: `s0ucipher scanned ${before.length} file(s) from 0 source(s). The strongest pattern is ${before.length ? strongestCategory : 'Mixed'}, and the planned output is NeuroSort Organized.`,
      safety_note: 'Preview mode is active, so files are not moved yet.',
      recommended_actions: recommendedActions,
      study_plan: [
        'Keep notes and assignments inside Study Hub.',
        'Open high-priority files first before exams.',
        'Use duplicate suggestions only for review, never automatic deletion.',
      ],
      automation_ideas: [
        'Suggest cleaner file names.',
        'Detect exam files and important study material.',
        'Separate media, documents, and uncategorized files.',
        'Warn before moving duplicates or old files.',
      ],
    },
  }
}

module.exports = {
  methodNotAllowed,
  organizeMetadata,
  parseJsonBody,
  webUnsupported,
}
