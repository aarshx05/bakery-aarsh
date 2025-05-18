import React from 'react';
import { Link } from 'react-router-dom';
import { ArrowRight } from 'lucide-react';

const Hero: React.FC = () => {
  return (
    <section className="relative h-screen flex items-center justify-center overflow-hidden">
      <div className="absolute inset-0 z-0">
        <img 
          src="https://images.pexels.com/photos/1028714/pexels-photo-1028714.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=2" 
          alt="Bakery" 
          className="w-full h-full object-cover"
        />
        <div className="absolute inset-0 bg-gradient-to-r from-brown-900/70 to-brown-800/50"></div>
      </div>
      
      <div className="container-custom relative z-10 text-center text-white px-4">
        <h2 className="text-4xl md:text-5xl lg:text-6xl font-playfair font-bold mb-4 drop-shadow-lg animate-fadeIn">
          Baked with Love, <br /> Served with Joy
        </h2>
        <p className="text-lg md:text-xl max-w-2xl mx-auto mb-8 drop-shadow-md">
          Indulge in our handcrafted breads, pastries, and cakes made fresh daily with the finest ingredients
        </p>
        <div className="flex flex-col sm:flex-row gap-4 justify-center">
          <Link 
            to="/products" 
            className="btn btn-primary text-lg flex items-center justify-center gap-2 py-3 px-8"
          >
            Order Now <ArrowRight className="w-5 h-5" />
          </Link>
          <Link 
            to="/about" 
            className="btn btn-outline text-lg border-cream-200 text-cream-100 hover:bg-cream-200 hover:text-brown-700"
          >
            Our Story
          </Link>
        </div>
      </div>
    </section>
  );
};

export default Hero;