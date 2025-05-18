import React, { useEffect, useState } from 'react';
import { MapPin, Phone, Mail, Clock } from 'lucide-react';

const Contact: React.FC = () => {
  useEffect(() => {
    window.scrollTo(0, 0);
  }, []);

  const [formData, setFormData] = useState({
    name: '',
    email: '',
    phone: '',
    message: '',
  });
  
  const [errors, setErrors] = useState<{[key: string]: string}>({});
  const [submitted, setSubmitted] = useState(false);
  
  const handleChange = (e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>) => {
    const { name, value } = e.target;
    setFormData(prev => ({ ...prev, [name]: value }));
    
    // Clear error when field is edited
    if (errors[name]) {
      setErrors(prev => {
        const newErrors = { ...prev };
        delete newErrors[name];
        return newErrors;
      });
    }
  };
  
  const validateForm = () => {
    const newErrors: {[key: string]: string} = {};
    
    if (!formData.name) newErrors.name = 'Name is required';
    if (!formData.email) {
      newErrors.email = 'Email is required';
    } else if (!/\S+@\S+\.\S+/.test(formData.email)) {
      newErrors.email = 'Email is invalid';
    }
    if (!formData.message) newErrors.message = 'Message is required';
    
    setErrors(newErrors);
    return Object.keys(newErrors).length === 0;
  };
  
  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    
    if (validateForm()) {
      // Simulate form submission
      setTimeout(() => {
        setSubmitted(true);
        setFormData({
          name: '',
          email: '',
          phone: '',
          message: '',
        });
      }, 1000);
    }
  };

  return (
    <div className="pt-24 pb-16">
      <div className="container-custom">
        <div className="text-center mb-16">
          <h1 className="text-3xl md:text-4xl font-playfair font-semibold text-brown-700 mb-4">
            Contact Us
          </h1>
          <p className="text-brown-500 max-w-2xl mx-auto">
            We'd love to hear from you! Whether you have a question about our products, need help with an order, or want to explore catering options.
          </p>
        </div>
        
        {/* Contact Cards */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-16">
          <div className="bg-white rounded-lg shadow-md p-6 text-center">
            <div className="bg-pink-100 rounded-full w-14 h-14 flex items-center justify-center mx-auto mb-4">
              <MapPin className="text-pink-500 w-6 h-6" />
            </div>
            <h3 className="font-playfair font-semibold text-brown-700 mb-2">Visit Us</h3>
            <p className="text-brown-600">
              123 Baker Street
              <br />
              Sweetville, NY 10001
            </p>
          </div>
          
          <div className="bg-white rounded-lg shadow-md p-6 text-center">
            <div className="bg-pink-100 rounded-full w-14 h-14 flex items-center justify-center mx-auto mb-4">
              <Phone className="text-pink-500 w-6 h-6" />
            </div>
            <h3 className="font-playfair font-semibold text-brown-700 mb-2">Call Us</h3>
            <p className="text-brown-600">
              (555) 123-4567
              <br />
              Customer Service: Ext. 1
            </p>
          </div>
          
          <div className="bg-white rounded-lg shadow-md p-6 text-center">
            <div className="bg-pink-100 rounded-full w-14 h-14 flex items-center justify-center mx-auto mb-4">
              <Mail className="text-pink-500 w-6 h-6" />
            </div>
            <h3 className="font-playfair font-semibold text-brown-700 mb-2">Email Us</h3>
            <p className="text-brown-600">
              hello@sweetcrumbs.com
              <br />
              orders@sweetcrumbs.com
            </p>
          </div>
          
          <div className="bg-white rounded-lg shadow-md p-6 text-center">
            <div className="bg-pink-100 rounded-full w-14 h-14 flex items-center justify-center mx-auto mb-4">
              <Clock className="text-pink-500 w-6 h-6" />
            </div>
            <h3 className="font-playfair font-semibold text-brown-700 mb-2">Opening Hours</h3>
            <p className="text-brown-600">
              Mon-Fri: 7am - 7pm
              <br />
              Sat-Sun: 8am - 5pm
            </p>
          </div>
        </div>
        
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-10 mb-16">
          {/* Map */}
          <div className="rounded-lg overflow-hidden shadow-lg h-96 bg-gray-200">
            <iframe 
              src="https://www.google.com/maps/embed?pb=!1m18!1m12!1m3!1d3022.9663095343035!2d-74.00425872516866!3d40.741894471388675!2m3!1f0!2f0!3f0!3m2!1i1024!2i768!4f13.1!3m3!1m2!1s0x89c259bf5c1654f3%3A0xc80f9cfce5383d5d!2sMadison%20Square%20Garden!5e0!3m2!1sen!2sus!4v1698101182266!5m2!1sen!2sus" 
              width="100%" 
              height="100%" 
              style={{ border: 0 }} 
              allowFullScreen={true} 
              loading="lazy" 
              referrerPolicy="no-referrer-when-downgrade"
              title="Sweet Crumbs Bakery Location"
            ></iframe>
          </div>
          
          {/* Contact Form */}
          <div className="bg-white rounded-lg shadow-lg p-8">
            <h2 className="text-2xl font-playfair font-semibold text-brown-700 mb-6">
              Send Us a Message
            </h2>
            
            {submitted ? (
              <div className="bg-green-50 border border-green-200 text-green-800 rounded-md p-4 mb-6">
                <p className="font-medium">Thank you for your message!</p>
                <p>We'll get back to you as soon as possible.</p>
              </div>
            ) : (
              <form onSubmit={handleSubmit} className="space-y-4">
                <div>
                  <label htmlFor="name" className="block text-brown-600 mb-1">
                    Name*
                  </label>
                  <input
                    type="text"
                    id="name"
                    name="name"
                    value={formData.name}
                    onChange={handleChange}
                    className={`input-field ${errors.name ? 'border-red-500' : ''}`}
                  />
                  {errors.name && (
                    <p className="text-red-500 text-sm mt-1">{errors.name}</p>
                  )}
                </div>
                
                <div>
                  <label htmlFor="email" className="block text-brown-600 mb-1">
                    Email*
                  </label>
                  <input
                    type="email"
                    id="email"
                    name="email"
                    value={formData.email}
                    onChange={handleChange}
                    className={`input-field ${errors.email ? 'border-red-500' : ''}`}
                  />
                  {errors.email && (
                    <p className="text-red-500 text-sm mt-1">{errors.email}</p>
                  )}
                </div>
                
                <div>
                  <label htmlFor="phone" className="block text-brown-600 mb-1">
                    Phone (Optional)
                  </label>
                  <input
                    type="tel"
                    id="phone"
                    name="phone"
                    value={formData.phone}
                    onChange={handleChange}
                    className="input-field"
                  />
                </div>
                
                <div>
                  <label htmlFor="message" className="block text-brown-600 mb-1">
                    Message*
                  </label>
                  <textarea
                    id="message"
                    name="message"
                    rows={5}
                    value={formData.message}
                    onChange={handleChange}
                    className={`input-field resize-none ${errors.message ? 'border-red-500' : ''}`}
                  ></textarea>
                  {errors.message && (
                    <p className="text-red-500 text-sm mt-1">{errors.message}</p>
                  )}
                </div>
                
                <button type="submit" className="btn btn-primary w-full">
                  Send Message
                </button>
              </form>
            )}
          </div>
        </div>
        
        {/* FAQ Section */}
        <div className="bg-cream-100 rounded-lg p-8 shadow-md">
          <div className="text-center mb-8">
            <h2 className="text-2xl md:text-3xl font-playfair font-semibold text-brown-700 mb-3">
              Frequently Asked Questions
            </h2>
            <p className="text-brown-500 max-w-2xl mx-auto">
              Find answers to our most commonly asked questions
            </p>
          </div>
          
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            <div className="bg-white p-6 rounded-lg shadow-sm">
              <h3 className="font-playfair font-semibold text-brown-700 mb-2">
                Do you offer gluten-free options?
              </h3>
              <p className="text-brown-600">
                Yes, we offer a selection of gluten-free breads, pastries, and cakes. However, please note that they are prepared in the same kitchen as our regular products.
              </p>
            </div>
            
            <div className="bg-white p-6 rounded-lg shadow-sm">
              <h3 className="font-playfair font-semibold text-brown-700 mb-2">
                How far in advance should I order a custom cake?
              </h3>
              <p className="text-brown-600">
                For custom cakes, we recommend placing your order at least 7 days in advance. For wedding cakes, please contact us at least 1-2 months ahead.
              </p>
            </div>
            
            <div className="bg-white p-6 rounded-lg shadow-sm">
              <h3 className="font-playfair font-semibold text-brown-700 mb-2">
                Can I place a large order for an event?
              </h3>
              <p className="text-brown-600">
                Absolutely! We offer catering services for events of all sizes. Please contact us directly for special orders and event catering.
              </p>
            </div>
            
            <div className="bg-white p-6 rounded-lg shadow-sm">
              <h3 className="font-playfair font-semibold text-brown-700 mb-2">
                What is your delivery radius?
              </h3>
              <p className="text-brown-600">
                We deliver within a 15-mile radius of our bakery. Delivery fees vary based on distance. For locations outside our delivery area, please contact us.
              </p>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Contact;