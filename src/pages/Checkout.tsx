import React, { useState, useEffect } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import { ArrowLeft } from 'lucide-react';
import { useCart } from '../context/CartContext';

const Checkout: React.FC = () => {
  const { cart, getTotalPrice, clearCart } = useCart();
  const navigate = useNavigate();
  
  const [formData, setFormData] = useState({
    firstName: '',
    lastName: '',
    email: '',
    phone: '',
    address: '',
    city: '',
    state: '',
    zipCode: '',
    orderType: 'pickup',
    pickupDate: '',
    pickupTime: '',
  });
  
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [errors, setErrors] = useState<{[key: string]: string}>({});
  
  useEffect(() => {
    window.scrollTo(0, 0);
    
    if (cart.length === 0) {
      navigate('/cart');
    }
  }, [cart, navigate]);
  
  const handleChange = (e: React.ChangeEvent<HTMLInputElement | HTMLSelectElement>) => {
    const { name, value } = e.target;
    setFormData(prev => ({ ...prev, [name]: value }));
    
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
    
    if (!formData.firstName) newErrors.firstName = 'First name is required';
    if (!formData.lastName) newErrors.lastName = 'Last name is required';
    if (!formData.email) {
      newErrors.email = 'Email is required';
    } else if (!/\S+@\S+\.\S+/.test(formData.email)) {
      newErrors.email = 'Email is invalid';
    }
    if (!formData.phone) newErrors.phone = 'Phone number is required';
    
    if (formData.orderType === 'delivery') {
      if (!formData.address) newErrors.address = 'Address is required';
      if (!formData.city) newErrors.city = 'City is required';
      if (!formData.state) newErrors.state = 'State is required';
      if (!formData.zipCode) newErrors.zipCode = 'ZIP code is required';
    }
    
    if (formData.orderType === 'pickup') {
      if (!formData.pickupDate) newErrors.pickupDate = 'Pickup date is required';
      if (!formData.pickupTime) newErrors.pickupTime = 'Pickup time is required';
    }
    
    setErrors(newErrors);
    return Object.keys(newErrors).length === 0;
  };
  
  const formatWhatsAppMessage = () => {
    const items = cart.map(item => 
      `• ${item.name} x${item.quantity} - ₹${(item.price * item.quantity)}`
    ).join('\n');

    const subtotal = getTotalPrice();
    const gst = subtotal * 0.18;
    const deliveryFee = formData.orderType === 'delivery' ? '(As per location)' : 'Free';
    const total = subtotal + gst;

    return encodeURIComponent(
      `*New Order from Sweet Crumbs Bakery*\n\n` +
      `*Customer Details:*\n` +
      `Name: ${formData.firstName} ${formData.lastName}\n` +
      `Email: ${formData.email}\n` +
      `Phone: ${formData.phone}\n\n` +
      `*Order Type:* ${formData.orderType.charAt(0).toUpperCase() + formData.orderType.slice(1)}\n` +
      (formData.orderType === 'delivery' 
        ? `Address: ${formData.address}, ${formData.city}, ${formData.state} ${formData.zipCode}\n(Delivery charges will be confirmed based on location)`
        : `Pickup Date: ${formData.pickupDate}\nPickup Time: ${formData.pickupTime}`
      ) + '\n\n' +
      `*Order Items:*\n${items}\n\n` +
      `*Order Summary:*\n` +
      `Subtotal: ₹${subtotal}\n` +
      `GST (18%): ₹${gst}\n` +
      `Delivery: ${deliveryFee}\n` +
      `*Total (without delivery): ₹${total}*`
    );
  };
  
  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    
    if (validateForm()) {
      setIsSubmitting(true);
      
      const message = formatWhatsAppMessage();
      const whatsappNumber = '919833254304'; // Replace with your WhatsApp Business number
      const whatsappUrl = `https://wa.me/${whatsappNumber}?text=${message}`;
      
      clearCart();
      window.location.href = whatsappUrl;
    }
  };
  
  const today = new Date().toISOString().split('T')[0];
  
  return (
    <div className="pt-28 pb-16">
      <div className="container-custom">
        <Link to="/cart" className="inline-flex items-center text-brown-600 hover:text-pink-500 mb-6">
          <ArrowLeft className="w-4 h-4 mr-1" /> Back to Cart
        </Link>
        
        <h1 className="text-3xl font-playfair font-semibold text-brown-700 mb-8">
          Checkout
        </h1>
        
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-8">
          {/* Checkout Form */}
          <div className="lg:col-span-2">
            <form onSubmit={handleSubmit} className="space-y-8">
              {/* Contact Information */}
              <div className="bg-white rounded-lg shadow-md p-6">
                <h2 className="text-xl font-playfair font-semibold text-brown-700 mb-4">
                  Contact Information
                </h2>
                <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                  <div>
                    <label htmlFor="firstName" className="block text-brown-600 mb-1">
                      First Name*
                    </label>
                    <input
                      type="text"
                      id="firstName"
                      name="firstName"
                      value={formData.firstName}
                      onChange={handleChange}
                      className={`input-field ${errors.firstName ? 'border-red-500' : ''}`}
                    />
                    {errors.firstName && (
                      <p className="text-red-500 text-sm mt-1">{errors.firstName}</p>
                    )}
                  </div>
                  <div>
                    <label htmlFor="lastName" className="block text-brown-600 mb-1">
                      Last Name*
                    </label>
                    <input
                      type="text"
                      id="lastName"
                      name="lastName"
                      value={formData.lastName}
                      onChange={handleChange}
                      className={`input-field ${errors.lastName ? 'border-red-500' : ''}`}
                    />
                    {errors.lastName && (
                      <p className="text-red-500 text-sm mt-1">{errors.lastName}</p>
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
                      Phone Number*
                    </label>
                    <input
                      type="tel"
                      id="phone"
                      name="phone"
                      value={formData.phone}
                      onChange={handleChange}
                      className={`input-field ${errors.phone ? 'border-red-500' : ''}`}
                    />
                    {errors.phone && (
                      <p className="text-red-500 text-sm mt-1">{errors.phone}</p>
                    )}
                  </div>
                </div>
              </div>
              
              {/* Order Type */}
              <div className="bg-white rounded-lg shadow-md p-6">
                <h2 className="text-xl font-playfair font-semibold text-brown-700 mb-4">
                  Order Type
                </h2>
                <div className="flex flex-wrap gap-4 mb-6">
                  <label className="flex items-center cursor-pointer">
                    <input
                      type="radio"
                      name="orderType"
                      value="pickup"
                      checked={formData.orderType === 'pickup'}
                      onChange={handleChange}
                      className="sr-only"
                    />
                    <span 
                      className={`px-4 py-2 rounded-full border-2 ${
                        formData.orderType === 'pickup'
                          ? 'border-pink-500 bg-pink-50 text-pink-500'
                          : 'border-cream-300 text-brown-600'
                      }`}
                    >
                      Pickup
                    </span>
                  </label>
                  <label className="flex items-center cursor-pointer">
                    <input
                      type="radio"
                      name="orderType"
                      value="delivery"
                      checked={formData.orderType === 'delivery'}
                      onChange={handleChange}
                      className="sr-only"
                    />
                    <span 
                      className={`px-4 py-2 rounded-full border-2 ${
                        formData.orderType === 'delivery'
                          ? 'border-pink-500 bg-pink-50 text-pink-500'
                          : 'border-cream-300 text-brown-600'
                      }`}
                    >
                      Delivery
                    </span>
                  </label>
                </div>
                
                {formData.orderType === 'pickup' && (
                  <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                    <div>
                      <label htmlFor="pickupDate" className="block text-brown-600 mb-1">
                        Pickup Date*
                      </label>
                      <input
                        type="date"
                        id="pickupDate"
                        name="pickupDate"
                        min={today}
                        value={formData.pickupDate}
                        onChange={handleChange}
                        className={`input-field ${errors.pickupDate ? 'border-red-500' : ''}`}
                      />
                      {errors.pickupDate && (
                        <p className="text-red-500 text-sm mt-1">{errors.pickupDate}</p>
                      )}
                    </div>
                    <div>
                      <label htmlFor="pickupTime" className="block text-brown-600 mb-1">
                        Pickup Time*
                      </label>
                      <select
                        id="pickupTime"
                        name="pickupTime"
                        value={formData.pickupTime}
                        onChange={handleChange}
                        className={`input-field ${errors.pickupTime ? 'border-red-500' : ''}`}
                      >
                        <option value="">Select a time</option>
                        <option value="8:00 AM">8:00 AM</option>
                        <option value="9:00 AM">9:00 AM</option>
                        <option value="10:00 AM">10:00 AM</option>
                        <option value="11:00 AM">11:00 AM</option>
                        <option value="12:00 PM">12:00 PM</option>
                        <option value="1:00 PM">1:00 PM</option>
                        <option value="2:00 PM">2:00 PM</option>
                        <option value="3:00 PM">3:00 PM</option>
                        <option value="4:00 PM">4:00 PM</option>
                        <option value="5:00 PM">5:00 PM</option>
                      </select>
                      {errors.pickupTime && (
                        <p className="text-red-500 text-sm mt-1">{errors.pickupTime}</p>
                      )}
                    </div>
                  </div>
                )}
                
                {formData.orderType === 'delivery' && (
                  <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                    <div className="md:col-span-2">
                      <label htmlFor="address" className="block text-brown-600 mb-1">
                        Address*
                      </label>
                      <input
                        type="text"
                        id="address"
                        name="address"
                        value={formData.address}
                        onChange={handleChange}
                        className={`input-field ${errors.address ? 'border-red-500' : ''}`}
                      />
                      {errors.address && (
                        <p className="text-red-500 text-sm mt-1">{errors.address}</p>
                      )}
                    </div>
                    <div>
                      <label htmlFor="city" className="block text-brown-600 mb-1">
                        City*
                      </label>
                      <input
                        type="text"
                        id="city"
                        name="city"
                        value={formData.city}
                        onChange={handleChange}
                        className={`input-field ${errors.city ? 'border-red-500' : ''}`}
                      />
                      {errors.city && (
                        <p className="text-red-500 text-sm mt-1">{errors.city}</p>
                      )}
                    </div>
                    <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
                      <div>
                        <label htmlFor="state" className="block text-brown-600 mb-1">
                          State*
                        </label>
                        <input
                          type="text"
                          id="state"
                          name="state"
                          value={formData.state}
                          onChange={handleChange}
                          className={`input-field ${errors.state ? 'border-red-500' : ''}`}
                        />
                        {errors.state && (
                          <p className="text-red-500 text-sm mt-1">{errors.state}</p>
                        )}
                      </div>
                      <div>
                        <label htmlFor="zipCode" className="block text-brown-600 mb-1">
                          ZIP Code*
                        </label>
                        <input
                          type="text"
                          id="zipCode"
                          name="zipCode"
                          value={formData.zipCode}
                          onChange={handleChange}
                          className={`input-field ${errors.zipCode ? 'border-red-500' : ''}`}
                        />
                        {errors.zipCode && (
                          <p className="text-red-500 text-sm mt-1">{errors.zipCode}</p>
                        )}
                      </div>
                    </div>
                  </div>
                )}
                
                {formData.orderType === 'delivery' && (
                  <p className="mt-4 text-sm text-brown-600">
                    * Delivery charges will be calculated based on your location and confirmed via WhatsApp.
                  </p>
                )}
              </div>
              
              <button
                type="submit"
                disabled={isSubmitting}
                className={`w-full btn btn-primary py-3 text-lg ${
                  isSubmitting ? 'opacity-70 cursor-not-allowed' : ''
                }`}
              >
                {isSubmitting ? 'Processing...' : 'Place Order'}
              </button>
            </form>
          </div>
          
          {/* Order Summary */}
          <div>
            <div className="bg-white rounded-lg shadow-md p-6 sticky top-28">
              <h2 className="text-xl font-playfair font-semibold text-brown-700 mb-4">
                Order Summary
              </h2>
              
              <div className="space-y-4 mb-6">
                {cart.map((item) => (
                  <div key={item.id} className="flex justify-between items-center">
                    <div className="flex items-center">
                      <img 
                        src={item.image} 
                        alt={item.name} 
                        className="w-12 h-12 object-cover rounded mr-3"
                      />
                      <div>
                        <h4 className="text-sm font-medium text-brown-700">
                          {item.name} <span className="text-brown-500">x{item.quantity}</span>
                        </h4>
                      </div>
                    </div>
                    <span className="text-brown-700">
                      ₹{(item.price * item.quantity)}
                    </span>
                  </div>
                ))}
              </div>
              
              <div className="border-t border-cream-200 pt-4 space-y-3">
                <div className="flex justify-between text-brown-600">
                  <span>Subtotal</span>
                  <span>₹{getTotalPrice()}</span>
                </div>
                <div className="flex justify-between text-brown-600">
                  <span>GST (18%)</span>
                  <span>₹{(getTotalPrice() * 0.18).toFixed(2)}</span>
                </div>
                <div className="flex justify-between text-brown-600">
                  <span>Delivery Fee</span>
                  <span>
                    {formData.orderType === 'delivery' ? 'As per location' : 'Free'}
                  </span>
                </div>
                <div className="pt-3 border-t border-cream-200 flex justify-between font-semibold text-lg text-brown-700">
                  <span>Total</span>
                  <span>
                    ₹{(getTotalPrice() * 1.18)}
                    {formData.orderType === 'delivery' && ' + Delivery'}
                  </span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Checkout