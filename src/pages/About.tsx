import React, { useEffect } from 'react';
import { Link } from 'react-router-dom';

const About: React.FC = () => {
  useEffect(() => {
    window.scrollTo(0, 0);
  }, []);

  return (
    <div className="pt-24 pb-16">
      <div className="container-custom">
        <div className="text-center mb-16">
          <h1 className="text-3xl md:text-4xl font-playfair font-semibold text-brown-700 mb-4">
            Our Story
          </h1>
          <p className="text-brown-500 max-w-2xl mx-auto">
            The journey of Sweet Crumbs Bakery from a small kitchen to a beloved community bakery
          </p>
        </div>
        
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-12 items-center mb-16">
          <div>
            <h2 className="text-2xl md:text-3xl font-playfair font-semibold text-brown-700 mb-4">
              How It All Began
            </h2>
            <p className="text-brown-600 mb-4">
              Sweet Crumbs Bakery was born from a passion for creating handcrafted baked goods using traditional techniques and the finest ingredients. What started as weekend baking in a small apartment kitchen in 2010 has grown into a beloved bakery serving our community with love in every bite.
            </p>
            <p className="text-brown-600 mb-4">
              Our founder, Sarah Miller, learned the art of baking from her grandmother, whose sourdough bread recipe—dating back three generations—continues to be our signature offering today. After years of sharing her creations with friends and family who encouraged her to turn her passion into a business, Sarah took the leap and opened the doors to Sweet Crumbs Bakery.
            </p>
            <p className="text-brown-600">
              From day one, our mission has remained the same: to create exceptional baked goods that bring joy and comfort, using time-honored methods and locally sourced ingredients whenever possible.
            </p>
          </div>
          <div className="rounded-lg overflow-hidden shadow-lg">
            <img 
              src="https://images.pexels.com/photos/960540/pexels-photo-960540.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=2" 
              alt="Baker kneading dough" 
              className="w-full h-auto"
            />
          </div>
        </div>
        
        <div className="bg-cream-100 rounded-lg p-8 md:p-12 shadow-md mb-16">
          <div className="text-center mb-8">
            <h2 className="text-2xl md:text-3xl font-playfair font-semibold text-brown-700 mb-4">
              Our Philosophy
            </h2>
          </div>
          
          <div className="grid grid-cols-1 md:grid-cols-3 gap-8">
            <div className="text-center">
              <div className="bg-white rounded-full w-16 h-16 flex items-center justify-center mx-auto mb-4 shadow-md">
                <span className="text-pink-500 text-2xl font-playfair">1</span>
              </div>
              <h3 className="text-xl font-playfair font-semibold text-brown-700 mb-2">Quality Ingredients</h3>
              <p className="text-brown-600">
                We believe that exceptional baked goods start with exceptional ingredients. We source organic flour, local dairy, and seasonal produce whenever possible.
              </p>
            </div>
            
            <div className="text-center">
              <div className="bg-white rounded-full w-16 h-16 flex items-center justify-center mx-auto mb-4 shadow-md">
                <span className="text-pink-500 text-2xl font-playfair">2</span>
              </div>
              <h3 className="text-xl font-playfair font-semibold text-brown-700 mb-2">Traditional Methods</h3>
              <p className="text-brown-600">
                We honor time-tested baking techniques, taking no shortcuts. Our bread is naturally leavened and given time to develop complex flavors.
              </p>
            </div>
            
            <div className="text-center">
              <div className="bg-white rounded-full w-16 h-16 flex items-center justify-center mx-auto mb-4 shadow-md">
                <span className="text-pink-500 text-2xl font-playfair">3</span>
              </div>
              <h3 className="text-xl font-playfair font-semibold text-brown-700 mb-2">Community Focus</h3>
              <p className="text-brown-600">
                We believe in giving back to the community that supports us, through donations, events, and partnerships with local organizations.
              </p>
            </div>
          </div>
        </div>
        
        <div className="mb-16">
          <div className="text-center mb-10">
            <h2 className="text-2xl md:text-3xl font-playfair font-semibold text-brown-700 mb-4">
              Meet Our Team
            </h2>
            <p className="text-brown-500 max-w-2xl mx-auto">
              The talented individuals who pour their hearts into creating your favorite baked goods
            </p>
          </div>
          
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-8">
            <div className="text-center">
              <div className="rounded-lg overflow-hidden mb-4">
                <img 
                  src="https://images.pexels.com/photos/4252137/pexels-photo-4252137.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=2" 
                  alt="Sarah Miller" 
                  className="w-full h-auto aspect-square object-cover"
                />
              </div>
              <h3 className="text-xl font-playfair font-semibold text-brown-700 mb-1">Sarah Miller</h3>
              <p className="text-pink-500 mb-2">Founder & Head Baker</p>
              <p className="text-brown-600 text-sm">
                Started Sweet Crumbs with her grandmother's recipes and a dream to share her passion for baking.
              </p>
            </div>
            
            <div className="text-center">
              <div className="rounded-lg overflow-hidden mb-4">
                <img 
                  src="https://images.pexels.com/photos/8477131/pexels-photo-8477131.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=2" 
                  alt="James Chen" 
                  className="w-full h-auto aspect-square object-cover"
                />
              </div>
              <h3 className="text-xl font-playfair font-semibold text-brown-700 mb-1">James Chen</h3>
              <p className="text-pink-500 mb-2">Pastry Chef</p>
              <p className="text-brown-600 text-sm">
                Trained in Paris, James brings artistic flair and technical precision to our pastry selection.
              </p>
            </div>
            
            <div className="text-center">
              <div className="rounded-lg overflow-hidden mb-4">
                <img 
                  src="https://images.pexels.com/photos/3771120/pexels-photo-3771120.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=2" 
                  alt="Elena Rodriguez" 
                  className="w-full h-auto aspect-square object-cover"
                />
              </div>
              <h3 className="text-xl font-playfair font-semibold text-brown-700 mb-1">Elena Rodriguez</h3>
              <p className="text-pink-500 mb-2">Bread Specialist</p>
              <p className="text-brown-600 text-sm">
                An expert in artisanal bread making with a passion for sourdough and ancient grain varieties.
              </p>
            </div>
            
            <div className="text-center">
              <div className="rounded-lg overflow-hidden mb-4">
                <img 
                  src="https://images.pexels.com/photos/3814446/pexels-photo-3814446.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=2" 
                  alt="Michael Thompson" 
                  className="w-full h-auto aspect-square object-cover"
                />
              </div>
              <h3 className="text-xl font-playfair font-semibold text-brown-700 mb-1">Michael Thompson</h3>
              <p className="text-pink-500 mb-2">Cake Decorator</p>
              <p className="text-brown-600 text-sm">
                Creates stunning custom cakes that are as beautiful as they are delicious.
              </p>
            </div>
          </div>
        </div>
        
        <div className="bg-brown-700 text-white rounded-lg overflow-hidden shadow-lg">
          <div className="grid grid-cols-1 lg:grid-cols-2">
            <div className="p-8 md:p-12 flex flex-col justify-center">
              <h2 className="text-2xl md:text-3xl font-playfair font-semibold text-gold-400 mb-4">
                Visit Us Today
              </h2>
              <p className="mb-6">
                We'd love to welcome you to our bakery. Come experience the aroma of freshly baked bread, watch our bakers at work, and taste the difference that passion and quality ingredients make.
              </p>
              <div className="flex space-x-4">
                <Link to="/products" className="btn btn-primary">
                  Order Online
                </Link>
                <Link to="/contact" className="btn btn-outline border-gold-400 text-gold-400 hover:bg-gold-400 hover:text-brown-800">
                  Find Us
                </Link>
              </div>
            </div>
            <div className="order-first lg:order-last">
              <img 
                src="https://images.pexels.com/photos/1855214/pexels-photo-1855214.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=2" 
                alt="Bakery storefront" 
                className="w-full h-full object-cover"
              />
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default About;