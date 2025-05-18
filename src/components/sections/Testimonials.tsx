import React from 'react';
import { Star } from 'lucide-react';

interface TestimonialProps {
  name: string;
  text: string;
  rating: number;
}

const Testimonial: React.FC<TestimonialProps> = ({ name, text, rating }) => {
  return (
    <div className="bg-white rounded-lg shadow-md p-6 flex flex-col h-full">
      <div className="flex mb-3">
        {[...Array(5)].map((_, i) => (
          <Star
            key={i}
            className={`w-5 h-5 ${i < rating ? 'text-gold-500 fill-gold-500' : 'text-cream-300'}`}
          />
        ))}
      </div>
      <p className="text-brown-600 italic flex-grow">{text}</p>
      <div className="mt-4 pt-4 border-t border-cream-200">
        <p className="font-semibold text-brown-700">{name}</p>
      </div>
    </div>
  );
};

const Testimonials: React.FC = () => {
  const testimonials = [
    {
      name: 'Emily Johnson',
      text: 'The sourdough bread from Sweet Crumbs is absolutely amazing! It has the perfect crust and chewy interior. I\'ve tried many bakeries, but this is by far the best bread in town.',
      rating: 5,
    },
    {
      name: 'Michael Rodriguez',
      text: 'I ordered a birthday cake for my daughter, and it exceeded all expectations. Not only was it beautiful, but it was also delicious. Everyone at the party wanted to know where it came from!',
      rating: 5,
    },
    {
      name: 'Sophia Chen',
      text: 'Their almond croissants are to die for! Perfectly flaky and not too sweet. I\'ve become a regular customer and always look forward to my Saturday morning treat.',
      rating: 4,
    },
  ];

  return (
    <section className="py-16 bg-cream-50">
      <div className="container-custom">
        <div className="text-center mb-12">
          <h2 className="text-3xl md:text-4xl font-playfair font-semibold text-brown-700 mb-3">
            What Our Customers Say
          </h2>
          <p className="text-brown-500 max-w-2xl mx-auto">
            We take pride in creating delicious baked goods that bring joy to our customers
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-8">
          {testimonials.map((testimonial, index) => (
            <Testimonial
              key={index}
              name={testimonial.name}
              text={testimonial.text}
              rating={testimonial.rating}
            />
          ))}
        </div>
      </div>
    </section>
  );
};

export default Testimonials;