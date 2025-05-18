/** @type {import('tailwindcss').Config} */
export default {
  content: ['./index.html', './src/**/*.{js,ts,jsx,tsx}'],
  theme: {
    extend: {
      colors: {
        cream: {
          50: '#FFFCF7',
          100: '#FDF6E9',
          200: '#F8ECD1',
          300: '#F2E0B9',
          400: '#EBCF92',
          500: '#E4BF6A',
        },
        brown: {
          50: '#F8F5F2',
          100: '#F0E9E1',
          200: '#DBC9B5',
          300: '#C3A989',
          400: '#AA895D',
          500: '#8B5A2B',
          600: '#73481F',
          700: '#5C3717',
          800: '#442510',
          900: '#2C1808',
        },
        pink: {
          50: '#FEF2F5',
          100: '#FDE6EB',
          200: '#FBCCD8',
          300: '#F8B4C4',
          400: '#F69CB0',
          500: '#F3849C',
          600: '#C26A7D',
          700: '#92505E',
          800: '#61353E',
          900: '#311B1F',
        },
        gold: {
          50: '#FBFAF5',
          100: '#F7F4EB',
          200: '#EEE8D2',
          300: '#E4DDB9',
          400: '#D7C985',
          500: '#CCA352',
          600: '#A38241',
          700: '#7B6231',
          800: '#524121',
          900: '#292110',
        },
      },
      fontFamily: {
        'playfair': ['"Playfair Display"', 'serif'],
        'poppins': ['Poppins', 'sans-serif'],
      },
      backgroundImage: {
        'hero-pattern': "url('https://images.pexels.com/photos/1028714/pexels-photo-1028714.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=2')",
      },
    },
  },
  plugins: [],
};