/** @type {import('tailwindcss').Config} */
module.exports = {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        obsidian: '#030712',
        cyan: {
          DEFAULT: '#00F5D4',
        },
        violet: {
          DEFAULT: '#7B2CBF',
        },
        surface: 'rgba(255, 255, 255, 0.05)',
      },
      fontFamily: {
        sans: ['Plus Jakarta Sans', 'sans-serif'],
        display: ['Syne', 'Outfit', 'sans-serif'],
        mono: ['JetBrains Mono', 'monospace'],
      },
      backdropBlur: {
        liquid: '28px',
      }
    },
  },
  plugins: [],
}
