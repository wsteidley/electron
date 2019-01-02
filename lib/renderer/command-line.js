'use strict'

exports.hasSwitch = function (name) {
  return process.argv.includes(`--${name}`)
}

exports.getSwitchValue = function (name, defaultValue, converter = value => value) {
  for (const arg of process.argv) {
    if (arg.indexOf(`--${name}=`) === 0) {
      return converter(arg.substr(arg.indexOf('=') + 1))
    }
  }
  return defaultValue
}
