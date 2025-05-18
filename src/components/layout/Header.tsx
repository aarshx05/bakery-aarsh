import React, { useState, useEffect } from 'react';
import { Link, useLocation } from 'react-router-dom';
import { Menu, X, ShoppingCart } from 'lucide-react';
import { useCart } from '../../context/CartContext';

const Header: React.FC = () => {
  const [isOpen, setIsOpen] = useState(false);
  const [isScrolled, setIsScrolled] = useState(false);
  const { getTotalItems } = useCart();
  const location = useLocation();
  const isHomePage = location.pathname === '/';
  const cartItemCount = getTotalItems();

  const toggleMenu = () => {
    setIsOpen(!isOpen);
  };

  useEffect(() => {
    const handleScroll = () => {
      if (window.scrollY > 50) {
        setIsScrolled(true);
      } else {
        setIsScrolled(false);
      }
    };

    window.addEventListener('scroll', handleScroll);
    return () => {
      window.removeEventListener('scroll', handleScroll);
    };
  }, []);

  useEffect(() => {
    setIsOpen(false);
  }, [location]);

  const headerClasses = `fixed w-full z-50 transition-all duration-300 ${
    isScrolled
      ? 'bg-white shadow-md py-2'
      : isHomePage
      ? 'bg-transparent py-4'
      : 'bg-brown-600 py-4'
  }`;

  const logoClasses = `font-playfair font-bold text-2xl md:text-3xl transition-colors duration-300 ${
    isScrolled ? 'text-brown-600' : 'text-white drop-shadow-md'
  }`;

  const navLinkClasses = `transition-colors duration-300 hover:text-pink-500 ${
    isScrolled ? 'text-brown-700' : 'text-white drop-shadow-md'
  }`;

  const mobileMenuClasses = `md:hidden fixed inset-0 bg-cream-100 z-50 transform transition-transform duration-300 ease-in-out ${
    isOpen ? 'translate-x-0' : 'translate-x-full'
  }`;

  return (
    <header className={headerClasses}>
      <div className="container-custom mx-auto flex justify-between items-center">
        <Link to="/" className={logoClasses}>
          Maillard Bakehouse
        </Link>

        {/* Desktop Navigation */}
        <nav className="hidden md:flex space-x-8 items-center">
          <Link to="/" className={navLinkClasses}>
            Home
          </Link>
          <Link to="/products" className={navLinkClasses}>
            Products
          </Link>
          <Link to="/about" className={navLinkClasses}>
            About
          </Link>
          <Link to="/contact" className={navLinkClasses}>
            Contact
          </Link>
          <Link to="/cart" className="relative">
            <ShoppingCart
              className={`w-6 h-6 ${
                isScrolled ? 'text-brown-700' : 'text-white'
              }`}
            />
            {cartItemCount > 0 && (
              <span className="absolute -top-2 -right-2 bg-pink-500 text-white text-xs font-bold rounded-full w-5 h-5 flex items-center justify-center">
                {cartItemCount}
              </span>
            )}
          </Link>
        </nav>

        {/* Mobile Menu Button */}
        <div className="md:hidden flex items-center">
          <Link to="/cart" className="relative mr-4">
            <ShoppingCart
              className={`w-6 h-6 ${
                isScrolled ? 'text-brown-700' : 'text-white'
              }`}
            />
            {cartItemCount > 0 && (
              <span className="absolute -top-2 -right-2 bg-pink-500 text-white text-xs font-bold rounded-full w-5 h-5 flex items-center justify-center">
                {cartItemCount}
              </span>
            )}
          </Link>
          <button
            onClick={toggleMenu}
            className={`p-2 focus:outline-none ${
              isScrolled ? 'text-brown-700' : 'text-white'
            }`}
          >
            <Menu className="w-6 h-6" />
          </button>
        </div>
      </div>

      {/* Mobile Menu */}
      <div className={mobileMenuClasses}>
        <div className="flex justify-end p-4">
          <button onClick={toggleMenu} className="p-2 focus:outline-none">
            <X className="w-6 h-6 text-brown-700" />
          </button>
        </div>
        <div className="flex flex-col items-center space-y-6 mt-20 text-2xl">
          <Link to="/" className="text-brown-700 hover:text-pink-500">
            Home
          </Link>
          <Link to="/products" className="text-brown-700 hover:text-pink-500">
            Products
          </Link>
          <Link to="/about" className="text-brown-700 hover:text-pink-500">
            About
          </Link>
          <Link to="/contact" className="text-brown-700 hover:text-pink-500">
            Contact
          </Link>
        </div>
      </div>
    </header>
  );
};

export default Header;
