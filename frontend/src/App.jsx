import { useState, useEffect, useMemo, useRef } from 'react'
import JSZip from 'jszip'
import axios from 'axios'
import {
  AlertTriangle,
  Archive,
  BookOpen,
  CheckCircle2,
  Download,
  Files,
  FolderOpen,
  FolderTree,
  Image,
  Play,
  Pause,
  RotateCcw,
  Settings,
  Keyboard,
  Moon,
  Sun,
  Shield,
  UploadCloud,
  Video,
  ChevronRight,
  Info,
  Plus
} from 'lucide-react'
import './index.css'
import { FAQ_QUESTIONS } from './faq'

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || ''

const categoryIcons = {
  Study: BookOpen,
  Images: Image,
  Documents: Files,
  Videos: Video,
  Others: Archive,
}

const categoryColors = {
  Study: 'violet',
  Images: 'blue',
  Documents: 'red',
  Videos: 'pink',
  Others: 'cyan',
}

export default function App() {
  const [envMode, setEnvMode] = useState('local')
  const [sources, setSources] = useState([])
  const [destinationPath, setDestinationPath] = useState('')
  const [saveMode, setSaveMode] = useState('downloads') // 'downloads' | 'custom'
  const [sortBy, setSortBy] = useState('name')
  const [applyChanges, setApplyChanges] = useState(false)
  const [dragActive, setDragActive] = useState(false)
  const [isDark, setIsDark] = useState(false) // Default to light mode (sky-blue theme)
  const [showInfoModal, setShowInfoModal] = useState(false)
  const [preScanResults, setPreScanResults] = useState(null)
  const [showSettingsModal, setShowSettingsModal] = useState(false)
  const [showShortcutsModal, setShowShortcutsModal] = useState(false)
  const [showDevModal, setShowDevModal] = useState(false)
  const [scanDelay, setScanDelay] = useState(30)
  const [duplicateCheck, setDuplicateCheck] = useState(true)

  // AI Chatbot State
  const [chatOpen, setChatOpen] = useState(false)
  const [chatSize, setChatSize] = useState('medium')
  const [chatTab, setChatTab] = useState('chat') // 'chat' | 'faq'
  const [chatMessages, setChatMessages] = useState([])
  const [chatInput, setChatInput] = useState('')
  const [faqSearch, setFaqSearch] = useState('')
  const [selectedFaqCategory, setSelectedFaqCategory] = useState('All')
  const [openFaqIndex, setOpenFaqIndex] = useState(null)
  
  const messagesEndRef = useRef(null)

  // Scanning simulation state
  const [scanState, setScanState] = useState('idle') // 'idle' | 'scanning' | 'paused' | 'complete'
  const [scanProgress, setScanProgress] = useState(0)
  const [elapsedTime, setElapsedTime] = useState('00:00')
  const [currentStep, setCurrentStep] = useState(1)
  const [loading, setLoading] = useState(false)

  // API Results
  const [results, setResults] = useState(null)
  const [saving, setSaving] = useState(false)
  const [saveResult, setSaveResult] = useState(null)
  const [error, setError] = useState(null)

  // Fetch env mode on mount
  useEffect(() => {
    const fetchConfig = async () => {
      try {
        const response = await axios.get(`${API_BASE_URL}/api/config`)
        if (response.data?.envMode) {
          setEnvMode(response.data.envMode)
        }
      } catch (err) {
        console.error('Could not fetch environment configuration', err)
      }
    }
    fetchConfig()
  }, [])

  // Manage body dark mode class
  useEffect(() => {
    if (isDark) {
      document.body.classList.add('dark-theme')
    } else {
      document.body.classList.remove('dark-theme')
    }
  }, [isDark])

  // Scroll chat to bottom
  useEffect(() => {
    if (chatOpen && chatTab === 'chat' && messagesEndRef.current) {
      setTimeout(() => {
        messagesEndRef.current.scrollIntoView({ behavior: 'smooth' })
      }, 50)
    }
  }, [chatMessages, chatOpen, chatTab])

  // Timer reference for pause/resume
  const [secondsElapsed, setSecondsElapsed] = useState(0)
  const [timerId, setTimerId] = useState(null)
  const [progressId, setProgressId] = useState(null)

  // Format bytes helper
  const formatSize = (bytes) => {
    if (!bytes || bytes === 0) return '0 Bytes'
    const k = 1024
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB']
    const i = Math.floor(Math.log(bytes) / Math.log(k))
    return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i]
  }

  // Total size calculated from selected sources (if files)
  const totalSelectedSize = useMemo(() => {
    let bytes = 0
    sources.forEach(src => {
      if (src.size) bytes += src.size
    })
    return formatSize(bytes)
  }, [sources])

  // Fetch folder picker
  const requestSelection = async (endpoint, responseKey, fallbackMessage) => {
    setError(null)
    try {
      const response = await axios.get(`${API_BASE_URL}${endpoint}`)
      return response.data[responseKey] || (responseKey === 'paths' ? [] : '')
    } catch (err) {
      const message = err.response?.data?.error || fallbackMessage
      if (!message.toLowerCase().includes('cancel')) {
        setError(message)
      }
      return responseKey === 'paths' ? [] : ''
    }
  }

  const handleChooseFiles = async () => {
    setError(null)
    if (envMode === 'web') {
      const fileInput = document.createElement('input')
      fileInput.type = 'file'
      fileInput.multiple = true
      fileInput.onchange = (e) => {
        if (e.target.files.length) {
          const arr = Array.from(e.target.files)
          addSources(arr)
        }
      }
      fileInput.click()
    } else {
      const paths = await requestSelection('/api/select-files', 'paths', 'Could not select files.')
      addSources(paths)
    }
  }

  const handleChooseFolders = async () => {
    setError(null)
    if (envMode === 'web') {
      const fileInput = document.createElement('input')
      fileInput.type = 'file'
      fileInput.multiple = true
      fileInput.webkitdirectory = true
      fileInput.onchange = (e) => {
        if (e.target.files.length) {
          const arr = Array.from(e.target.files)
          addSources(arr)
        }
      }
      fileInput.click()
    } else {
      const paths = await requestSelection('/api/select-folders', 'paths', 'Could not select folders.')
      addSources(paths)
    }
  }

  const handleChooseDestination = async () => {
    setError(null)
    const path = await requestSelection('/api/select-destination', 'path', 'Could not select destination.')
    if (path) {
      setDestinationPath(path)
    }
  }

  const addSources = (newItems) => {
    if (!newItems || newItems.length === 0) return
    setSources((prev) => {
      const isNewLocal = typeof newItems[0] === 'string'
      const isPrevLocal = prev.length > 0 && typeof prev[0] === 'string'
      
      let baseList = prev
      if (prev.length > 0 && isNewLocal !== isPrevLocal) {
        baseList = []
        setError("Switched selection source: Local paths and browser files cannot be mixed. sources are reset to the new type.")
      }
      
      const merged = [...baseList, ...newItems]
      const unique = []
      const seen = new Set()
      for (const item of merged) {
        const key = typeof item === 'string' ? item : (item.customPath || item.webkitRelativePath || item.name)
        if (!seen.has(key)) {
          seen.add(key)
          unique.push(item)
        }
      }
      return unique
    })
  }

  const handleRemoveSource = (item) => {
    setSources((prev) => prev.filter((s) => s !== item))
  }

  const handleClearSources = () => {
    setSources([])
    setResults(null)
    setScanState('idle')
    setScanProgress(0)
    setSecondsElapsed(0)
    setElapsedTime('00:00')
    setSaveResult(null)
    setError(null)
  }

  // Drag and Drop
  const handleDrag = (e) => {
    e.preventDefault()
    e.stopPropagation()
    if (e.type === "dragenter" || e.type === "dragover") {
      setDragActive(true)
    } else if (e.type === "dragleave") {
      setDragActive(false)
    }
  }

  const handleDrop = async (e) => {
    e.preventDefault()
    e.stopPropagation()
    setDragActive(false)
    
    if (e.dataTransfer.items && e.dataTransfer.items.length > 0) {
      const items = Array.from(e.dataTransfer.items)
      const filePromises = []
      
      const traverseEntry = (entry, path = '') => {
        return new Promise((resolve) => {
          if (entry.isFile) {
            entry.file((file) => {
              // Attach custom path for recursive directories
              file.customPath = path ? `${path}/${file.name}` : file.name
              resolve([file])
            }, () => resolve([]))
          } else if (entry.isDirectory) {
            const dirReader = entry.createReader()
            
            const readAllEntries = () => {
              return new Promise((resEntries) => {
                const allEntries = []
                const readBatch = () => {
                  dirReader.readEntries(async (entries) => {
                    if (entries.length === 0) {
                      resEntries(allEntries)
                    } else {
                      allEntries.push(...entries)
                      readBatch()
                    }
                  }, () => resEntries(allEntries))
                }
                readBatch()
              })
            }
            
            readAllEntries().then(async (entries) => {
              const nestedPromises = entries.map(e => traverseEntry(e, path ? `${path}/${entry.name}` : entry.name))
              const results = await Promise.all(nestedPromises)
              resolve(results.flat())
            })
          } else {
            resolve([])
          }
        })
      }
      
      for (const item of items) {
        if (item.kind === 'file') {
          const entry = item.webkitGetAsEntry()
          if (entry) {
            filePromises.push(traverseEntry(entry))
          }
        }
      }
      
      const fileGroups = await Promise.all(filePromises)
      const flatFiles = fileGroups.flat()
      if (flatFiles.length > 0) {
        addSources(flatFiles)
      }
    } else if (e.dataTransfer.files && e.dataTransfer.files[0]) {
      const arr = Array.from(e.dataTransfer.files)
      addSources(arr)
    }
  }

  // Trigger scanning simulation
  const startScanning = async () => {
    if (!sources.length) return
    setError(null)
    setResults(null)
    setPreScanResults(null) // Reset pre-scan results
    setSaveResult(null)
    setScanState('scanning')
    setScanProgress(0)
    setSecondsElapsed(0)
    setElapsedTime('00:00')
    setCurrentStep(1)
    setLoading(true)

    // Stopwatch logic
    let currentSeconds = 0
    const tid = setInterval(() => {
      currentSeconds++
      setSecondsElapsed(currentSeconds)
      const min = String(Math.floor(currentSeconds / 60)).padStart(2, '0')
      const sec = String(currentSeconds % 60).padStart(2, '0')
      setElapsedTime(`${min}:${sec}`)
    }, 1000)
    setTimerId(tid)

    // Call API
    let apiResponse = null
    try {
      let payload = {}
      if (envMode === 'web' || (sources.length > 0 && typeof sources[0] !== 'string')) {
        payload = {
          files_metadata: sources.map((file) => ({
            name: file.name,
            size_bytes: file.size || 0,
            lastModified: file.lastModified || Date.now(),
            path: file.customPath || file.webkitRelativePath || file.name,
          })),
          sortBy,
        }
      } else {
        payload = {
          sources,
          destinationPath: saveMode === 'custom' ? destinationPath : '',
          sortBy,
          applyChanges,
        }
      }
      const response = await axios.post(`${API_BASE_URL}/api/organize`, payload)
      apiResponse = response.data
      setPreScanResults(apiResponse) // Set immediately to feed sizes/counts into animation
    } catch (err) {
      console.error(err)
      setError(err.response?.data?.error || 'Scan connection failed. Check server execution on port 5050.')
    }

    // Progress Bar Animation
    let progressVal = 0
    const pid = setInterval(() => {
      progressVal += 1
      setScanProgress(progressVal)
      
      // Checklist step state thresholds
      if (progressVal < 25) {
        setCurrentStep(1)
      } else if (progressVal >= 25 && progressVal < 50) {
        setCurrentStep(2)
      } else if (progressVal >= 50 && progressVal < 80) {
        setCurrentStep(3)
      } else if (progressVal >= 80 && progressVal < 100) {
        setCurrentStep(4)
      } else if (progressVal >= 100) {
        clearInterval(pid)
        clearInterval(tid)
        setScanState('complete')
        setLoading(false)
        if (apiResponse) {
          setResults(apiResponse)
        } else {
          setScanState('idle')
        }
      }
    }, scanDelay) // Custom scan simulation delay
    setProgressId(pid)
  }

  const pauseScanning = () => {
    if (scanState !== 'scanning') return
    setScanState('paused')
    clearInterval(timerId)
    clearInterval(progressId)
  }

  const resumeScanning = () => {
    if (scanState !== 'paused') return
    setScanState('scanning')

    // Resume stopwatch
    let currentSeconds = secondsElapsed
    const tid = setInterval(() => {
      currentSeconds++
      setSecondsElapsed(currentSeconds)
      const min = String(Math.floor(currentSeconds / 60)).padStart(2, '0')
      const sec = String(currentSeconds % 60).padStart(2, '0')
      setElapsedTime(`${min}:${sec}`)
    }, 1000)
    setTimerId(tid)

    // Resume progress
    let progressVal = scanProgress
    const pid = setInterval(() => {
      progressVal += 1
      setScanProgress(progressVal)
      if (progressVal < 25) {
        setCurrentStep(1)
      } else if (progressVal >= 25 && progressVal < 50) {
        setCurrentStep(2)
      } else if (progressVal >= 50 && progressVal < 80) {
        setCurrentStep(3)
      } else if (progressVal >= 80 && progressVal < 100) {
        setCurrentStep(4)
      } else if (progressVal >= 100) {
        clearInterval(pid)
        clearInterval(tid)
        setScanState('complete')
        setLoading(false)
      }
    }, scanDelay)
    setProgressId(pid)
  }

  // Handle Export / Save organized copy
  const handleExportSave = async () => {
    if (!results) return
    setSaving(true)
    setError(null)
    setSaveResult(null)

    const isWebUpload = sources.length > 0 && typeof sources[0] !== 'string'

    if (envMode === 'web' || isWebUpload) {
      // Browser Zip download
      try {
        const zip = new JSZip()
        for (const record of results.before || []) {
          const matchedFile = sources.find((file) => {
            const path = file.customPath || file.webkitRelativePath || file.name
            return path === record.source || file.name === record.name
          })

          if (matchedFile) {
            const folderName = record.folder || 'Others'
            let zipFilePath = `${folderName}/${record.name}`
            let base = record.name
            let ext = ''
            const lastDot = record.name.lastIndexOf('.')
            if (lastDot !== -1) {
              base = record.name.substring(0, lastDot)
              ext = record.name.substring(lastDot)
            }
            
            let counter = 1
            while (zip.file(zipFilePath)) {
              zipFilePath = `${folderName}/${base} (${counter})${ext}`
              counter++
            }
            zip.file(zipFilePath, matchedFile)
          }
        }

        const content = await zip.generateAsync({ type: 'blob' })
        const url = URL.createObjectURL(content)
        const link = document.createElement('a')
        link.href = url
        link.download = `NeuroSort_Organized_${new Date().toISOString().slice(0, 10)}.zip`
        document.body.appendChild(link)
        link.click()
        document.body.removeChild(link)
        URL.revokeObjectURL(url)

        setSaveResult({
          assistant_message: `Successfully compiled and zipped ${results.before?.length || 0} file(s).`,
          export_path: 'Downloaded directly via your web browser.',
        })
      } catch (err) {
        console.error(err)
        setError('Could not compile and download the ZIP archive.')
      } finally {
        setSaving(false)
      }
    } else {
      // Local system copy
      try {
        const payload = {
          sources: results.sources || [],
          sortBy: results.sort_by || 'name',
          destinationPath: saveMode === 'custom' ? destinationPath : '',
          saveMode: saveMode,
        }
        const response = await axios.post(`${API_BASE_URL}/api/save-organized`, payload)
        setSaveResult(response.data)
      } catch (err) {
        console.error(err)
        setError(err.response?.data?.error || 'Failed to save organized directories.')
      } finally {
        setSaving(false)
      }
    }
  }

  // UI calculations
  const workspacePath = useMemo(() => {
    if (!sources.length) return 'No folder selected'
    if (typeof sources[0] === 'string') {
      return sources[0]
    }
    return 'Web Sandbox Session'
  }, [sources])

  const stats = useMemo(() => {
    if (scanState === 'scanning' || scanState === 'paused') {
      // Count files up to the current progress percentage
      const total = preScanResults?.dashboard?.total_files || sources.length
      const count = Math.min(total, Math.floor((scanProgress / 100) * total))
      
      // Calculate folder counts up to progress
      const targetFolders = Math.max(1, Math.min(5, Math.ceil((scanProgress / 100) * 5)))
      
      // Size calculation
      let sizeBytes = 0
      if (preScanResults && preScanResults.before) {
        preScanResults.before.slice(0, count).forEach((file) => {
          sizeBytes += file.size_bytes || 0
        })
      } else {
        sources.slice(0, count).forEach((s) => {
          if (s.size) sizeBytes += s.size
        })
      }

      return {
        files: count.toLocaleString(),
        folders: targetFolders,
        size: formatSize(sizeBytes),
      }
    }

    if (results && results.dashboard) {
      let totalBytes = 0
      if (results.before) {
        results.before.forEach(r => { totalBytes += r.size_bytes })
      }
      return {
        files: results.dashboard.total_files.toLocaleString(),
        folders: results.dashboard.total_categories || 1,
        size: formatSize(totalBytes),
      }
    }

    return {
      files: '0',
      folders: '0',
      size: '0 Bytes',
    }
  }, [scanState, scanProgress, sources, results])

  // Category values percentages
  const categoriesList = useMemo(() => {
    const list = [
      { name: 'Photos', key: 'Images', sub: 'Images', icon: Image, color: 'blue' },
      { name: 'Videos', key: 'Videos', sub: 'Files', icon: Video, color: 'pink' },
      { name: 'PDFs', key: 'Documents', sub: 'Documents', icon: Files, color: 'red' },
      { name: 'Study Hub', key: 'Study', sub: 'Study Files', icon: BookOpen, color: 'violet' },
      { name: 'Others', key: 'Others', sub: 'Files', icon: Archive, color: 'cyan' },
    ]

    return list.map(item => {
      let count = 0
      if (results && results.categories && results.categories[item.key]) {
        count = results.categories[item.key].length
      }
      
      // calculate progress bar percentage
      const total = results?.dashboard?.total_files || 1
      const percentage = Math.round((count / total) * 100)

      return {
        ...item,
        count,
        percentage,
      }
    })
  }, [results])

  // Priorities values
  const prioritiesData = useMemo(() => {
    let importantCount = 0
    let lessImportantCount = 0
    let importantSize = 0
    let lessImportantSize = 0

    if (results && results.before) {
      results.before.forEach(file => {
        if (file.priority === 'High' || file.priority === 'Medium') {
          importantCount++
          importantSize += file.size_bytes
        } else {
          lessImportantCount++
          lessImportantSize += file.size_bytes
        }
      })
    }

    const total = results?.dashboard?.total_files || 1
    const importantPercent = Math.round((importantCount / total) * 100)
    const lessImportantPercent = Math.round((lessImportantCount / total) * 100)

    return {
      important: {
        count: importantCount,
        size: formatSize(importantSize),
        percentage: importantPercent,
      },
      lessImportant: {
        count: lessImportantCount,
        size: formatSize(lessImportantSize),
        percentage: lessImportantPercent,
      }
    }
  }, [results])

  const handleAskQuestion = (question, answer) => {
    setChatMessages((prev) => [...prev, { sender: 'user', text: question }])
    setTimeout(() => {
      setChatMessages((prev) => [...prev, { sender: 'ai', text: answer }])
    }, 200)
  }

  const handleChatSubmit = (e, overrideQuery) => {
    if (e && e.preventDefault) e.preventDefault()
    const query = (overrideQuery || chatInput).trim()
    if (!query) return
    if (!overrideQuery) setChatInput('')
    
    setChatMessages((prev) => [...prev, { sender: 'user', text: query }])
    
    setTimeout(() => {
      const lowerQuery = query.toLowerCase()
      let aiText = ''
      
      if (lowerQuery.includes('high priority') || lowerQuery.includes('important file') || lowerQuery.includes('which is high')) {
        if (!results || !results.before) {
          aiText = "You haven't run a file scan yet. Please choose some folders/files and click Start so I can analyze high priority files."
        } else {
          const highPrio = results.before.filter(f => f.priority === 'High' || f.priority === 'Medium')
          if (highPrio.length === 0) {
            aiText = "I found 0 high or medium priority files in this workspace."
          } else {
            aiText = `I identified ${highPrio.length} important (High/Medium priority) files. Here are the top items:\n` + 
              highPrio.slice(0, 10).map(f => `• ${f.name} (Category: ${f.folder || f.category}, Size: ${formatSize(f.size_bytes)})`).join('\n')
            if (highPrio.length > 10) aiText += `\n...and ${highPrio.length - 10} more files.`
          }
        }
      } else if (lowerQuery.includes('low priority') || lowerQuery.includes('less important') || lowerQuery.includes('which is low')) {
        if (!results || !results.before) {
          aiText = "Please perform a scan first so I can categorize low priority items."
        } else {
          const lowPrio = results.before.filter(f => f.priority === 'Low')
          if (lowPrio.length === 0) {
            aiText = "I found 0 low priority files."
          } else {
            aiText = `I identified ${lowPrio.length} low priority files. Here are the top items:\n` + 
              lowPrio.slice(0, 10).map(f => `• ${f.name} (Category: ${f.folder || f.category}, Size: ${formatSize(f.size_bytes)})`).join('\n')
            if (lowPrio.length > 10) aiText += `\n...and ${lowPrio.length - 10} more files.`
          }
        }
      } else if (lowerQuery.includes('how much data') || lowerQuery.includes('total size') || lowerQuery.includes('size') || lowerQuery.includes('space')) {
        if (!results) {
          let bytes = 0
          sources.forEach(src => { if (src.size) bytes += src.size })
          aiText = `Currently, you have selected ${sources.length} items with a total metadata size of ${formatSize(bytes)}. Run a scan to see their recursive layout size.`
        } else {
          aiText = `The scanned workspace contains a total file size of ${stats.size} across ${stats.files} files.`
        }
      } else if (lowerQuery.includes('category') || lowerQuery.includes('folders') || lowerQuery.includes('categories') || lowerQuery.includes('breakdown') || lowerQuery.includes('list')) {
        if (!results) {
          aiText = "Scan your folder to get a complete category breakdown of Photos, Videos, PDFs, and Study Hub files."
        } else {
          const catLines = categoriesList.map(cat => `• ${cat.name}: ${cat.count} files (${cat.percentage}%)`)
          aiText = `Here is the breakdown of the categories s0ucipher sorted:\n` + catLines.join('\n')
        }
      } else {
        let bestMatch = null
        let maxOverlap = 0
        const queryWords = lowerQuery.split(/\s+/)
        
        for (const faq of FAQ_QUESTIONS) {
          let overlap = 0
          const faqWords = faq.question.toLowerCase().split(/\s+/)
          queryWords.forEach(qw => {
            if (qw.length > 3 && faqWords.some(fw => fw.includes(qw) || qw.includes(fw))) {
              overlap++
            }
          })
          if (overlap > maxOverlap) {
            maxOverlap = overlap
            bestMatch = faq
          }
        }
        
        if (bestMatch && maxOverlap > 0) {
          aiText = `[Matched FAQ: "${bestMatch.question}"]\n\n${bestMatch.answer}`
        } else {
          aiText = "I couldn't find a direct FAQ answer for that query. Try asking something like 'which is high priority', 'how much data', or search the FAQ Library list in the FAQ tab!"
        }
      }
      
      setChatMessages((prev) => [...prev, { sender: 'ai', text: aiText }])
    }, 450)
  }

  return (
    <div className="dashboard-app">
      {/* Sidebar navigation */}
      <aside className="sidebar">
        <div className="brand-header">
          <div className="brand-logo">
            <span>N</span>
          </div>
          <h1>NeuroSort AI</h1>
        </div>

        {/* Sources controller */}
        <div className="sidebar-section">
          <h2>Source</h2>
          <div className="action-buttons-group">
            <button
              className="btn btn-primary-blue"
              onClick={handleChooseFiles}
              disabled={scanState === 'scanning'}
            >
              <Files size={17} /> Add Files
            </button>
            <button
              className="btn btn-primary-purple"
              onClick={handleChooseFolders}
              disabled={scanState === 'scanning'}
            >
              <FolderOpen size={17} /> Add Folders
            </button>
          </div>

          {/* Drag & Drop space */}
          <div
            className={`drag-drop-zone ${dragActive ? 'drag-active' : ''}`}
            onDragEnter={handleDrag}
            onDragOver={handleDrag}
            onDragLeave={handleDrag}
            onDrop={handleDrop}
          >
            <UploadCloud size={28} />
            <p>Drag & drop files or folders here</p>
            <span>You can add multiple items in a batch</span>
          </div>

          {/* Selected items count & reset */}
          {sources.length > 0 && (
            <div className="selected-summary">
              <div>
                <strong>{sources.length} items</strong>
                <span>Size: {totalSelectedSize}</span>
              </div>
              <button className="clear-btn" onClick={handleClearSources}>
                <RotateCcw size={16} /> Reset
              </button>
            </div>
          )}
        </div>

        {/* Save location options */}
        <div className="sidebar-section">
          <h2>Save Location</h2>
          <div className="choice-cards-group">
            <label className={`choice-card ${saveMode === 'downloads' ? 'selected' : ''}`}>
              <input
                type="radio"
                name="saveMode"
                value="downloads"
                checked={saveMode === 'downloads'}
                onChange={() => setSaveMode('downloads')}
              />
              <span className="radio-circle"></span>
              <div className="card-label">
                <strong>Save to Downloads</strong>
                <span>{envMode === 'web' ? 'Download ZIP via browser' : 'Default Location'}</span>
              </div>
            </label>

            <label className={`choice-card ${saveMode === 'custom' ? 'selected' : ''} ${envMode === 'web' ? 'disabled' : ''}`}>
              <input
                type="radio"
                name="saveMode"
                value="custom"
                checked={saveMode === 'custom'}
                disabled={envMode === 'web'}
                onChange={() => {
                  setSaveMode('custom')
                  handleChooseDestination()
                }}
              />
              <span className="radio-circle"></span>
              <div className="card-label">
                <strong>Choose Save Location</strong>
                <span>{destinationPath ? 'Target: custom directory' : 'Select a custom folder'}</span>
              </div>
            </label>
          </div>

          {destinationPath && saveMode === 'custom' && (
            <div className="destination-preview">
              <span>{destinationPath}</span>
            </div>
          )}
        </div>

        {/* s0ucipher AI Agent Info Card */}
        <div className="info-trigger-card" onClick={() => setShowInfoModal(true)}>
          <div className="info-trigger-header">
            <Info size={16} />
            <strong>What is s0ucipher AI?</strong>
          </div>
          <span>Learn how it organizes files</span>
        </div>

        {/* Security badge */}
        <div className="security-badge-card">
          <Shield size={18} />
          <div>
            <strong>Secure. Private. Local First.</strong>
            <span>Your files stay on your device.</span>
          </div>
        </div>

        {/* Bottom controls */}
        <div className="sidebar-footer">
          <button className="sidebar-tool-btn" onClick={() => setShowSettingsModal(true)}>
            <Settings size={19} />
            <span>Settings</span>
          </button>
          <button className="sidebar-tool-btn theme-btn" onClick={() => setIsDark(!isDark)}>
            {isDark ? <Sun size={19} /> : <Moon size={19} />}
            <span>{isDark ? 'Light' : 'Dark'}</span>
          </button>
          <button className="sidebar-tool-btn shortcuts-btn" onClick={() => setShowShortcutsModal(true)}>
            <Keyboard size={19} />
            <span>Shortcuts</span>
          </button>
        </div>
      </aside>

      {/* Main Container */}
      <main className="main-content">
        {/* Workspace bar */}
        <div className="workspace-bar">
          <div className="workspace-path">
            <span>Scanner Workspace</span>
            <strong>{workspacePath}</strong>
          </div>

          <div className="workspace-controls">
            {scanState === 'scanning' ? (
              <span className="status-indicator animate-pulse font-bold text-blue-500">
                • Scanning...
              </span>
            ) : scanState === 'paused' ? (
              <span className="status-indicator text-yellow-500">
                • Paused
              </span>
            ) : scanState === 'complete' ? (
              <span className="status-indicator text-green-500">
                • Ready
              </span>
            ) : (
              <span className="status-indicator text-gray-400">
                • Idle
              </span>
            )}

            {scanState === 'scanning' ? (
              <button className="btn-control" onClick={pauseScanning}>
                <Pause size={15} /> Pause
              </button>
            ) : scanState === 'paused' ? (
              <button className="btn-control active-start" onClick={resumeScanning}>
                <Play size={15} /> Resume
              </button>
            ) : (
              <button
                className="btn-control active-start"
                disabled={sources.length === 0}
                onClick={startScanning}
              >
                <Play size={15} /> Start
              </button>
            )}
          </div>
        </div>

        {/* Error panel */}
        {error && (
          <div className="error-alert">
            <AlertTriangle size={20} />
            <p>{error}</p>
          </div>
        )}

        {/* Analysis middle block */}
        <div className="analysis-summary-grid">
          {/* s0ucipher AI block */}
          <section className="summary-card agent-card">
            <div className="agent-profile">
              <div className="agent-avatar">
                <img src="/s0ucipher_avatar.png" alt="s0ucipher Avatar" className={scanState === 'scanning' ? 'pulse-avatar' : ''} />
              </div>
              <div className="agent-meta">
                <div className="agent-badge-row">
                  <h2>s0ucipher</h2>
                  <span className="badge-purple">AI Agent</span>
                </div>
                <p>Analyzing, understanding and organizing your files intelligently.</p>
              </div>
            </div>

            <div className="agent-stats">
              <div className="stat-item">
                <span>Files Found</span>
                <strong>{stats.files}</strong>
              </div>
              <div className="stat-item">
                <span>Folders</span>
                <strong>{stats.folders}</strong>
              </div>
              <div className="stat-item font-mono">
                <span>Size</span>
                <strong>{stats.size}</strong>
              </div>
              <div className="stat-item font-mono">
                <span>Elapsed</span>
                <strong>{elapsedTime}</strong>
              </div>
            </div>
          </section>

          {/* Live analysis checklist */}
          <section className="summary-card analysis-card">
            <div className="card-header">
              <h3>Live Analysis</h3>
              <p>Reading file types, contents and context...</p>
            </div>

            <div className="progress-bar-container">
              <div className="progress-bar" style={{ width: `${scanProgress}%` }}></div>
              <span className="progress-percentage">{scanProgress}%</span>
            </div>

            <ul className="analysis-steps-list">
              <li className={currentStep > 1 || scanState === 'complete' ? 'checked' : scanState === 'scanning' && currentStep === 1 ? 'active' : ''}>
                <span className="checkbox"></span>
                Scanning files and folders
              </li>
              <li className={currentStep > 2 || scanState === 'complete' ? 'checked' : scanState === 'scanning' && currentStep === 2 ? 'active' : ''}>
                <span className="checkbox"></span>
                Understanding file types
              </li>
              <li className={currentStep > 3 || scanState === 'complete' ? 'checked' : scanState === 'scanning' && currentStep === 3 ? 'active' : ''}>
                <span className="checkbox"></span>
                Analyzing content & importance
              </li>
              <li className={scanState === 'complete' ? 'checked' : scanState === 'scanning' && currentStep === 4 ? 'active' : ''}>
                <span className="checkbox"></span>
                Organizing & prioritizing
              </li>
            </ul>
          </section>
        </div>

        {/* Categories panel */}
        <section className="dashboard-section">
          <div className="section-header">
            <h3>Categories</h3>
            <button className="view-all-btn">
              View All <ChevronRight size={16} />
            </button>
          </div>

          <div className="categories-grid">
            {categoriesList.map((cat) => {
              const IconComp = cat.icon
              return (
                <div className={`category-card border-${cat.color}`} key={cat.name}>
                  <div className="cat-card-header">
                    <span className={`cat-icon bg-${cat.color}-light text-${cat.color}`}>
                      <IconComp size={20} />
                    </span>
                    <span className="cat-title">{cat.name}</span>
                  </div>
                  <div className="cat-card-body">
                    <strong>{results ? cat.count : '-'}</strong>
                    <span>{cat.sub}</span>
                  </div>
                  <div className="cat-progress-container">
                    <div
                      className={`cat-progress bg-${cat.color}`}
                      style={{ width: results ? `${cat.percentage}%` : '0%' }}
                    ></div>
                  </div>
                </div>
              )
            })}
          </div>
        </section>

        {/* Priority analysis panel */}
        <section className="dashboard-section">
          <h3>Priority Analysis</h3>
          <p className="section-description">
            s0ucipher evaluates importance based on content, recency and relevance.
          </p>

          <div className="priorities-grid">
            {/* High Priority card */}
            <div className={`priority-card important-card ${results ? 'active' : ''}`}>
              <div className="priority-header">
                <span className="radio-select"></span>
                <div className="priority-label">
                  <strong>Important</strong>
                  <span className="badge-high">High Priority</span>
                </div>
                <span className="priority-percentage font-mono">{results ? `${prioritiesData.important.percentage}%` : '0%'}</span>
              </div>
              <div className="priority-stats font-mono">
                {results ? `${prioritiesData.important.count} files • ${prioritiesData.important.size}` : '- files • - Bytes'}
              </div>
              <div className="priority-progress-bar-container">
                <div
                  className="priority-progress-bar bg-purple"
                  style={{ width: results ? `${prioritiesData.important.percentage}%` : '0%' }}
                ></div>
              </div>
            </div>

            {/* Low Priority card */}
            <div className={`priority-card less-important-card ${results ? 'active' : ''}`}>
              <div className="priority-header">
                <span className="radio-select"></span>
                <div className="priority-label">
                  <strong>Less Important</strong>
                  <span className="badge-low">Low Priority</span>
                </div>
                <span className="priority-percentage font-mono">{results ? `${prioritiesData.lessImportant.percentage}%` : '0%'}</span>
              </div>
              <div className="priority-stats font-mono">
                {results ? `${prioritiesData.lessImportant.count} files • ${prioritiesData.lessImportant.size}` : '- files • - Bytes'}
              </div>
              <div className="priority-progress-bar-container">
                <div
                  className="priority-progress-bar bg-gray"
                  style={{ width: results ? `${prioritiesData.lessImportant.percentage}%` : '0%' }}
                ></div>
              </div>
            </div>
          </div>
        </section>

        {/* Results preview table */}
        <section className="dashboard-section">
          <h3>Results Preview <span className="text-sm font-normal text-gray-500 font-mono">(Top Items)</span></h3>
          <div className="table-container">
            <table className="results-table">
              <thead>
                <tr>
                  <th>Name</th>
                  <th>Category</th>
                  <th>Importance</th>
                  <th>Size</th>
                  <th>Date Modified</th>
                  <th>AI Score</th>
                </tr>
              </thead>
              <tbody>
                {results && results.before && results.before.length > 0 ? (
                  results.before.map((file) => (
                    <tr key={file.source}>
                      <td className="file-name-cell">
                        <Files size={15} />
                        <span>{file.name}</span>
                      </td>
                      <td>
                        <span className={`pill-badge pill-${categoryColors[file.category] || 'cyan'}`}>
                          {file.folder || file.category}
                        </span>
                      </td>
                      <td>
                        <span className={`pill-badge pill-priority-${file.priority.toLowerCase().replace(' ', '-')}`}>
                          {file.priority}
                        </span>
                      </td>
                      <td className="font-mono text-sm text-gray-700">
                        {formatSize(file.size_bytes)}
                      </td>
                      <td className="font-mono text-sm text-gray-500">
                        {file.modified_at ? file.modified_at.replace('T', ' ') : '-'}
                      </td>
                      <td className="font-mono font-bold" style={{ color: 'var(--color-primary-purple)' }}>
                        {file.confidence ? `${Math.round(file.confidence * 100)}%` : '0%'}
                      </td>
                    </tr>
                  ))
                ) : (
                  <tr>
                    <td colSpan="6" className="empty-table-row">
                      <Info size={16} /> Scan sources to load preview items
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>
        </section>

        {/* Export Save Actions Block */}
        {results && (
          <div className="export-action-banner">
            <div className="export-meta">
              <CheckCircle2 size={24} className="text-green-500" />
              <div>
                <h4>Scanned & Sorted Successfully</h4>
                <p>
                  Ready to copy organized folders to target: {' '}
                  <strong>{saveMode === 'downloads' ? 'Downloads Folder' : destinationPath}</strong>
                </p>
              </div>
            </div>
            <button className="btn btn-save" onClick={handleExportSave} disabled={saving}>
              {saving ? <span className="spinner" /> : <Download size={18} />}
              {saving ? 'Exporting...' : 'Export & Save Copy'}
            </button>
          </div>
        )}

        {/* Save confirmation modal toast */}
        {saveResult && (
          <div className="save-toast-success">
            <CheckCircle2 size={20} />
            <div>
              <strong>{saveResult.assistant_message}</strong>
              <span>Target: {saveResult.export_path}</span>
            </div>
          </div>
        )}
      </main>

      {/* s0ucipher AI Agent Modal */}
      {showInfoModal && (
        <div className="info-modal-overlay" onClick={() => setShowInfoModal(false)}>
          <div className="info-modal-card" onClick={(e) => e.stopPropagation()}>
            <button className="close-modal-btn" onClick={() => setShowInfoModal(false)}>
              ×
            </button>
            <div className="info-modal-header">
              <div className="agent-avatar modal-avatar">
                <img src="/s0ucipher_avatar.png" alt="s0ucipher Avatar" />
              </div>
              <div>
                <h2>s0ucipher AI Agent</h2>
                <span className="badge-purple">Core Intelligence</span>
              </div>
            </div>
            
            <div className="info-modal-body">
              <p className="modal-description">
                s0ucipher is a local-first file analysis and organization agent. It scans your folders recursively and runs classification algorithms based on naming patterns, file suffixes, and exam dates.
              </p>
              
              <div className="info-features-grid">
                <div className="info-feature-item">
                  <strong>📁 Smart Classification</strong>
                  <p>Groups items into Photos, Videos, PDFs, and Study Hub directories.</p>
                </div>
                <div className="info-feature-item">
                  <strong>🎓 Academic Priority</strong>
                  <p>Detects keywords like "notes" or "exam" and flags them as High/Medium importance.</p>
                </div>
                <div className="info-feature-item">
                  <strong>🛡️ Duplicate Prevention</strong>
                  <p>Identifies duplicate filenames and flags cleanup suggestions.</p>
                </div>
                <div className="info-feature-item">
                  <strong>🔒 100% Secure & Local</strong>
                  <p>Written in native C. Files never leave your local device.</p>
                </div>
              </div>

              <div className="demo-visual-container">
                <h4>System Architecture & Flow</h4>
                <img src="/s0ucipher_demo.png" alt="s0ucipher AI scan flow diagram" className="demo-visual-img" />
              </div>
            </div>
          </div>
        </div>
      )}

      {/* Keyboard Shortcuts Modal */}
      {showShortcutsModal && (
        <div className="info-modal-overlay" onClick={() => setShowShortcutsModal(false)}>
          <div className="info-modal-card shortcuts-modal-card" onClick={(e) => e.stopPropagation()}>
            <button className="close-modal-btn" onClick={() => setShowShortcutsModal(false)}>×</button>
            <div className="modal-header-simple">
              <Keyboard size={22} className="text-blue" />
              <h2>Keyboard Shortcuts</h2>
            </div>
            <p style={{ fontSize: '0.82rem', color: 'var(--text-muted)', marginBottom: '20px' }}>
              Power-user shortcuts to navigate NeuroSort AI faster.
            </p>
            <div className="shortcuts-grid">
              <div className="shortcuts-group">
                <h4>📁 File Operations</h4>
                <div className="shortcut-row"><kbd>A</kbd><span>Add Files</span></div>
                <div className="shortcut-row"><kbd>F</kbd><span>Add Folder</span></div>
                <div className="shortcut-row"><kbd>R</kbd><span>Reset / Clear Selection</span></div>
                <div className="shortcut-row"><kbd>Space</kbd><span>Start Scan</span></div>
                <div className="shortcut-row"><kbd>P</kbd><span>Pause / Resume Scan</span></div>
              </div>
              <div className="shortcuts-group">
                <h4>🎨 Interface</h4>
                <div className="shortcut-row"><kbd>T</kbd><span>Toggle Light / Dark Theme</span></div>
                <div className="shortcut-row"><kbd>,</kbd><span>Open Settings</span></div>
                <div className="shortcut-row"><kbd>?</kbd><span>Open Shortcuts Panel</span></div>
                <div className="shortcut-row"><kbd>Esc</kbd><span>Close Any Modal</span></div>
              </div>
              <div className="shortcuts-group">
                <h4>🤖 AI Chatbot</h4>
                <div className="shortcut-row"><kbd>C</kbd><span>Open / Close Chatbot</span></div>
                <div className="shortcut-row"><kbd>H</kbd><span>Ask High Priority Files</span></div>
                <div className="shortcut-row"><kbd>L</kbd><span>Ask Low Priority Files</span></div>
              </div>
              <div className="shortcuts-group">
                <h4>💾 Export</h4>
                <div className="shortcut-row"><kbd>E</kbd><span>Export & Save (when ready)</span></div>
                <div className="shortcut-row"><kbd>D</kbd><span>Select Downloads Folder</span></div>
              </div>
            </div>
            <div className="shortcuts-tip">
              <span>💡</span>
              <p>All shortcuts are active when no text input is focused. s0ucipher AI processes everything locally — your files stay private.</p>
            </div>
          </div>
        </div>
      )}

      {/* Settings Modal */}
      {showSettingsModal && (
        <div className="info-modal-overlay" onClick={() => setShowSettingsModal(false)}>
          <div className="info-modal-card settings-modal-card" onClick={(e) => e.stopPropagation()}>
            <button className="close-modal-btn" onClick={() => setShowSettingsModal(false)}>×</button>
            <div className="modal-header-simple">
              <Settings size={22} className="text-blue" />
              <h2>Application Settings</h2>
            </div>
            <div className="settings-grid">
              <div className="setting-row">
                <label>
                  <strong>Work Mode</strong>
                  <span>Switch between local filesystem access and in-browser sandboxing.</span>
                </label>
                <select value={envMode} onChange={(e) => setEnvMode(e.target.value)}>
                  <option value="local">Local System Disk (macOS Pickers)</option>
                  <option value="web">Web Browser Sandbox (ZIP Exports)</option>
                </select>
              </div>
              <div className="setting-row">
                <label>
                  <strong>App Theme</strong>
                  <span>Choose the visual theme style.</span>
                </label>
                <div className="theme-toggle-buttons">
                  <button className={`btn-theme-choice ${!isDark ? 'active' : ''}`} onClick={() => setIsDark(false)}>
                    <Sun size={15} /> Light Theme (Sky-Blue)
                  </button>
                  <button className={`btn-theme-choice ${isDark ? 'active' : ''}`} onClick={() => setIsDark(true)}>
                    <Moon size={15} /> Dark Theme (Space-Cyber)
                  </button>
                </div>
              </div>
              <div className="setting-row">
                <label>
                  <strong>Default Sort Key</strong>
                  <span>How files should be ordered by default.</span>
                </label>
                <select value={sortBy} onChange={(e) => setSortBy(e.target.value)}>
                  <option value="name">File Name</option>
                  <option value="size">File Size (Descending)</option>
                  <option value="date">Date Modified</option>
                  <option value="priority">Priority Importance</option>
                </select>
              </div>
              <div className="setting-row">
                <label>
                  <strong>Scan Simulation Speed</strong>
                  <span>Control how fast the live checklist and progress bar ticks.</span>
                </label>
                <select value={scanDelay} onChange={(e) => setScanDelay(Number(e.target.value))}>
                  <option value="5">Super Fast (Instant)</option>
                  <option value="30">Normal (3 Seconds)</option>
                  <option value="100">Thorough Analysis (10 Seconds)</option>
                </select>
              </div>
              <div className="setting-row">
                <label>
                  <strong>Local Safe Mode</strong>
                  <span>(Local Mode only) Check duplicates before executing file movements.</span>
                </label>
                <input type="checkbox" checked={duplicateCheck} onChange={(e) => setDuplicateCheck(e.target.checked)} />
              </div>
              <div className="setting-row">
                <label>
                  <strong>Auto-Apply Changes</strong>
                  <span>(Local Mode only) Move files immediately on scanning instead of dry-run.</span>
                </label>
                <input type="checkbox" checked={applyChanges} onChange={(e) => setApplyChanges(e.target.checked)} />
              </div>
            </div>
            <div className="modal-footer-simple">
              <button className="btn btn-primary-blue" onClick={() => setShowSettingsModal(false)}>
                Save & Close
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Floating s0ucipher AI Chatbot */}
      <div className={`chatbot-widget ${chatOpen ? 'chat-expanded' : 'chat-collapsed'} size-${chatSize}`}>
        {chatOpen ? (
          <div className="chatbot-panel">
            {/* Header */}
            <div className="chat-header">
              <div className="chat-header-profile">
                <img src="/s0ucipher_avatar.png" alt="s0ucipher AI" />
                <div>
                  <h4>s0ucipher AI Agent</h4>
                  <span>Online Assistant</span>
                </div>
              </div>
              <div className="chat-header-actions">
                <button 
                  className="chat-action-btn size-toggle" 
                  title="Toggle Panel Size"
                  type="button"
                  onClick={() => {
                    if (chatSize === 'small') setChatSize('medium')
                    else if (chatSize === 'medium') setChatSize('large')
                    else setChatSize('small')
                  }}
                >
                  {chatSize.toUpperCase()}
                </button>
                <button className="chat-action-btn close-btn" type="button" onClick={() => setChatOpen(false)}>
                  ×
                </button>
              </div>
            </div>

            {/* Chat Tabs selector */}
            <div className="chat-tabs-bar">
              <button 
                type="button" 
                className={`chat-tab-btn ${chatTab === 'chat' ? 'active' : ''}`}
                onClick={() => setChatTab('chat')}
              >
                💬 Ask Assistant
              </button>
              <button 
                type="button" 
                className={`chat-tab-btn ${chatTab === 'faq' ? 'active' : ''}`}
                onClick={() => setChatTab('faq')}
              >
                📚 100+ FAQ Library
              </button>
            </div>

            {chatTab === 'chat' ? (
              <>
                {/* Chat message history */}
                <div className="chat-messages-container">
                  {chatMessages.length === 0 ? (
                    <div className="chat-empty-state">
                      <img src="/s0ucipher_avatar.png" alt="s0ucipher" className="chat-avatar-large" />
                      <h5>Ask s0ucipher AI</h5>
                      <p>I can help you review your scanned documents, check high/low priorities, calculate data size, and answer over 100 preset questions!</p>
                    </div>
                  ) : (
                    chatMessages.map((msg, idx) => (
                      <div key={idx} className={`chat-message ${msg.sender}-message`}>
                        <div className="message-bubble">{msg.text}</div>
                      </div>
                    ))
                  )}
                  <div ref={messagesEndRef} />
                </div>

                {/* Quick suggestions */}
                <div className="chat-quick-suggestions">
                  <button type="button" className="suggestion-chip" onClick={() => handleChatSubmit(null, "Which files are high priority?")}>
                    🔥 High Priority?
                  </button>
                  <button type="button" className="suggestion-chip" onClick={() => handleChatSubmit(null, "How much data is scanned?")}>
                    📊 Total Size?
                  </button>
                  <button type="button" className="suggestion-chip" onClick={() => handleChatSubmit(null, "Show category breakdown")}>
                    📁 Category list?
                  </button>
                </div>

                {/* Chat Input form */}
                <form className="chat-input-form" onSubmit={(e) => handleChatSubmit(e)}>
                  <input 
                    type="text" 
                    placeholder="Ask about priorities, size, or type a query..." 
                    value={chatInput}
                    onChange={(e) => setChatInput(e.target.value)}
                  />
                  <button type="submit" className="chat-send-btn">Send</button>
                </form>
              </>
            ) : (
              /* Full FAQ explorer */
              <div className="chat-faq-explorer">
                <div className="faq-explorer-header">
                  <h5>Search & Browse FAQ reference database</h5>
                  <div className="faq-search-filters">
                    <select value={selectedFaqCategory} onChange={(e) => setSelectedFaqCategory(e.target.value)}>
                      <option value="All">All Categories</option>
                      <option value="AI & Priority Rules">AI Priority Rules</option>
                      <option value="File Categories">File Categories</option>
                      <option value="Privacy & Security">Privacy & Security</option>
                      <option value="C Backend Internals">C Backend Internals</option>
                      <option value="Sorting & Structure">Sorting & Structure</option>
                      <option value="Troubleshooting">Troubleshooting</option>
                    </select>
                    <input 
                      type="text" 
                      placeholder="Type keywords to filter..." 
                      value={faqSearch}
                      onChange={(e) => setFaqSearch(e.target.value)}
                    />
                  </div>
                </div>

                <div className="faq-explorer-list">
                  {FAQ_QUESTIONS
                    .filter(q => selectedFaqCategory === 'All' || q.category === selectedFaqCategory)
                    .filter(q => q.question.toLowerCase().includes(faqSearch.toLowerCase()) || q.answer.toLowerCase().includes(faqSearch.toLowerCase()))
                    .map((q, idx) => {
                      const isOpen = openFaqIndex === idx;
                      return (
                        <div key={idx} className={`faq-accordion-item ${isOpen ? 'open' : ''}`}>
                          <button 
                            type="button" 
                            className="faq-accordion-trigger" 
                            onClick={() => setOpenFaqIndex(isOpen ? null : idx)}
                          >
                            <span className="faq-q-category">{q.category}</span>
                            <strong className="faq-q-text">{q.question}</strong>
                          </button>
                          {isOpen && (
                            <div className="faq-accordion-content">
                              <p>{q.answer}</p>
                              <div className="faq-accordion-actions">
                                <button 
                                  type="button" 
                                  className="btn-ask-chat"
                                  onClick={() => {
                                    handleAskQuestion(q.question, q.answer)
                                    setChatTab('chat')
                                  }}
                                >
                                  💬 Send Answer to Chat
                                </button>
                              </div>
                            </div>
                          )}
                        </div>
                      )
                    })}
                </div>
              </div>
            )}
          </div>
        ) : (
          <button className="chat-trigger-bubble" onClick={() => {
            setChatOpen(true)
            if (chatMessages.length === 0) {
              setChatMessages([
                { sender: 'ai', text: 'Hello! I am s0ucipher AI. Ask me anything about your files, priority classifications, or our high-performance C backend!' }
              ])
            }
          }}>
            <img src="/s0ucipher_avatar.png" alt="s0ucipher Chat Trigger" />
            <span className="chat-tooltip">Ask s0ucipher AI</span>
          </button>
        )}
      </div>
    </div>
  )
}
