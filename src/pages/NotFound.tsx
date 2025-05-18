import React from 'react';
import { Link } from 'react-router-dom';
import { ArrowLeft } from 'lucide-react';

const NotFound: React.FC = () => {
  return (
    <div className="pt-28 pb-16 min-h-screen flex items-center">
      <div className="container-custom text-center">
        <div className="max-w-md mx-auto">
          <h1 className="text-8xl font-playfair font-bold text-pink-400 mb-4">404</h1>
          <h2 className="text-2xl font-playfair font-semibold text-brown-700 mb-4">
            Page Not Found
          </h2>
          <p className="text-brown-500 mb-8">
            The page you're looking for doesn't seem to exist. Perhaps you'd like to try one of our freshly baked pages instead?
          </p>
          <Link to="/" className="btn btn-primary inline-flex items-center">
            <ArrowLeft className="w-4 h-4 mr-2" /> Back to Home
          </Link>
        </div>
      </div>
    </div>
  );
};

export default NotFound;