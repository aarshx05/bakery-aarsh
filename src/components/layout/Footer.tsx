import React from 'react';
import { Link } from 'react-router-dom';
import {
  MapPin,
  Phone,
  Mail,
  Clock,
  Instagram,
  Facebook,
  Twitter,
} from 'lucide-react';

const Footer: React.FC = () => {
  return (
    <footer className="bg-brown-600 text-cream-100 pt-12 pb-6">
      <div className="container-custom">
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-8">
          {/* About */}
          <div>
            <h3 className="text-2xl font-playfair text-gold-400 mb-4">
              Maillard Bakehouse
            </h3>
            <p className="mb-4">Create. Share. Savour.</p>
            <div className="flex space-x-4">
              <a
                href="https://www.instagram.com/maillard_bakehouse/"
                className="text-cream-100 hover:text-pink-300 transition-colors"
              >
                <Instagram className="w-5 h-5" />
              </a>
              <a
                href="https://www.facebook.com/people/Maillard-Bakehouse/pfbid0hSrM4mEfpe3UzN4qdyo5hijM8iw9v1jbD9KSS3hM9K4hWproNtMZaRpGaicdT9GPl/"
                className="text-cream-100 hover:text-pink-300 transition-colors"
              >
                <Facebook className="w-5 h-5" />
              </a>
              <a
                href="https://twitter.com"
                className="text-cream-100 hover:text-pink-300 transition-colors"
              >
                <Twitter className="w-5 h-5" />
              </a>
            </div>
          </div>

          {/* Quick Links */}
          <div>
            <h3 className="text-xl font-playfair text-gold-400 mb-4">
              Quick Links
            </h3>
            <ul className="space-y-2">
              <li>
                <Link to="/" className="hover:text-pink-300 transition-colors">
                  Home
                </Link>
              </li>
              <li>
                <Link
                  to="/products"
                  className="hover:text-pink-300 transition-colors"
                >
                  Products
                </Link>
              </li>
              <li>
                <Link
                  to="/about"
                  className="hover:text-pink-300 transition-colors"
                >
                  About Us
                </Link>
              </li>
              <li>
                <Link
                  to="/contact"
                  className="hover:text-pink-300 transition-colors"
                >
                  Contact
                </Link>
              </li>
            </ul>
          </div>

          {/* Contact Info */}
          <div>
            <h3 className="text-xl font-playfair text-gold-400 mb-4">
              Contact Us
            </h3>
            <ul className="space-y-3">
              <li className="flex items-start">
                <MapPin className="w-5 h-5 mr-2 mt-1 flex-shrink-0" />
                <span>
                  Unit B-217, Second floor, B-wing, Gokul Industrial Estate
                </span>
              </li>
              <li className="flex items-center">
                <Phone className="w-5 h-5 mr-2 flex-shrink-0" />
                <span>+91 7378500085</span>
              </li>
              <li className="flex items-center">
                <Mail className="w-5 h-5 mr-2 flex-shrink-0" />
                <span>hello@sweetcrumbs.com</span>
              </li>
            </ul>
          </div>

          {/* Hours */}
          <div>
            <h3 className="text-xl font-playfair text-gold-400 mb-4">
              Opening Hours
            </h3>
            <ul className="space-y-2">
              <li className="flex items-start">
                <Clock className="w-5 h-5 mr-2 mt-1 flex-shrink-0" />
                <div>
                  <p>Monday - Saturday</p>
                  <p>9:00 AM - 9:00 PM</p>
                </div>
              </li>
              <li className="flex items-start">
                <Clock className="w-5 h-5 mr-2 mt-1 flex-shrink-0" />
                <div>
                  <p>Sunday</p>
                  <p>Closed</p>
                </div>
              </li>
            </ul>
          </div>
        </div>

        <div className="border-t border-brown-500 mt-8 pt-6 text-center text-sm">
          <p>
            &copy; {new Date().getFullYear()} Maillard Bakehouse. All rights
            reserved.
          </p>
        </div>
      </div>
    </footer>
  );
};

export default Footer;
