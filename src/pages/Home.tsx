import React, { useEffect } from 'react';
import Hero from '../components/sections/Hero';
import FeaturedProducts from '../components/sections/FeaturedProducts';
import CategoriesSection from '../components/sections/CategoriesSection';
import Testimonials from '../components/sections/Testimonials';

const Home: React.FC = () => {
  useEffect(() => {
    window.scrollTo(0, 0);
  }, []);

  return (
    <div>
      <Hero />
      <FeaturedProducts />
      <div className="py-16 bg-pink-50">
        <div className="container-custom">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-8 items-center">
            <div>
              <h2 className="text-3xl md:text-4xl font-playfair font-semibold text-brown-700 mb-4">
                Artisanal Baking with a Touch of Love
              </h2>
              <p className="text-brown-600 mb-4">
                At Sweet Crumbs Bakery, we believe that every bite should be extraordinary. Our dedicated team of bakers wakes up at dawn to prepare fresh, artisanal goods using time-honored techniques and the finest ingredients.
              </p>
              <p className="text-brown-600 mb-6">
                Whether you're celebrating a special occasion or simply treating yourself to a morning pastry, we pour our passion into everything we create.
              </p>
              <button className="btn btn-secondary">Visit Our Bakery</button>
            </div>
            <div className="rounded-lg overflow-hidden shadow-lg">
              <img 
                src="https://images.pexels.com/photos/6941040/pexels-photo-6941040.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=2" 
                alt="Baker working" 
                className="w-full h-auto"
              />
            </div>
          </div>
        </div>
      </div>
      <CategoriesSection />
      <Testimonials />
    </div>
  );
};

export default Home;